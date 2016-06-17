#include <cmath>
using namespace std;

#include <WF.h>
using namespace NICE;

#include <Units.h>
using namespace UNIC;

#include "Reader.h"

Reader::Reader(int run, int sub, const char *dir) :
WFs(run), Logger(), fRaw(0)
{
   fPath = Form("%s/%04d00", dir, run/100);
   fName = Form("run_%06d.%06d", run, sub);
   Index();
   Initialize();
}

//------------------------------------------------------------------------------

void Reader::LoadIndex()
{
   const char *name=Form("%s/%s.idx", fPath.Data(), fName.Data());
   ifstream idx(name, ios::in);
   if (!idx.is_open()) {
      Info("LoadIndex","no %s",name);
      return;
   }
   Info("LoadIndex","%s",name);

   size_t begin;
   size_t size;
   fBegin.resize(0);
   fSize.resize(0);
   while(idx>>begin>>size) {
      fBegin.push_back(begin);
      fSize.push_back(size);
   }

   idx.close();
}

//------------------------------------------------------------------------------

void Reader::Index()
{
   // open data file in a read-only mode and put the "get" pointer to the end
   // of the file
   fRaw = new ifstream((fPath+"/"+fName).Data(), ios::binary | ios::ate); 
   if (!fRaw->is_open()) { //stream is not properly connected to the file
      Error("Index", "cannot read %s/%s, return", fPath.Data(), fName.Data()); 
      return; 
   } else {
      Info("Index", "%s/%s", fPath.Data(), fName.Data()); 
   }

   // get the position of the "get" pointer, since the "get" pointer is at the
   // end of the file, its position equals the size of the data file
   size_t fileSize = fRaw->tellg();
   Info("Index","file size: %zu Byte", fileSize);

   // check file size
   if (fileSize<sizeof(CAEN_DGTZ_LHEADER_t)) {
      Error("Index", "%s/%s is too small, return", fPath.Data(), fName.Data()); 
      fRaw->close();
      return;
   }

   LoadIndex();
   if (fBegin.size()>0) return;

   // move the "get" pointer back to the beginning of the file, from where
   // reading starts
   fRaw->seekg(0, ios::beg);

   // get total number of events and
   // save the starting points and sizes of events
   fBegin.resize(0);
   fSize.resize(0);
   while (fRaw->good() && size_t(fRaw->tellg())<fileSize) {
      //save starting points
      fBegin.push_back(fRaw->tellg());

      // save event size
      CAEN_DGTZ_LHEADER_t hdr;
      fRaw->read(reinterpret_cast<char *> (&hdr), sizeof(CAEN_DGTZ_LHEADER_t));
      if (hdr.evtSize*size_t(4)<sizeof(CAEN_DGTZ_LHEADER_t)) {
         fSize.push_back(hdr.evtSize*4);
         Error("Index", "entry %zu is too small, stop indexing", fBegin.size());
         break;
      }
      fSize.push_back(hdr.evtSize*4);

      //skip the rest of the block
      fRaw->seekg(hdr.evtSize*4-sizeof(CAEN_DGTZ_LHEADER_t), ios::cur);
   }
}

//------------------------------------------------------------------------------

void Reader::DumpIndex()
{
   for (size_t i=0; i<fBegin.size(); i++)
      Printf("%10zu %zu", fBegin[i], fSize[i]);
}

//------------------------------------------------------------------------------

void Reader::GetEntry(int i)
{
   fRaw->seekg(fBegin[i], ios::beg);

   while (size_t(fRaw->tellg())<fBegin[i]+fSize[i]) {
      CAEN_DGTZ_LHEADER_t hdr;
      fRaw->read(reinterpret_cast<char *> (&hdr), sizeof(CAEN_DGTZ_LHEADER_t));

      if (hdr.reserved2==1) {
         ReadRunInfo(i);
      } else if (hdr.reserved2==0) {
         evt = hdr.eventCount;
         cnt = hdr.TTimeTag;
         ReadEvent(i);
      } else {
         Warning("GetEntry", "unknown entry %d, skipped", i);
         fRaw->seekg(fSize[i]-sizeof(CAEN_DGTZ_LHEADER_t),ios::cur);
         return;
      }
   }
}

//------------------------------------------------------------------------------

void Reader::ReadRunInfo(int i)
{
   char *content = new char[fSize[i]-sizeof(CAEN_DGTZ_LHEADER_t)];
   fRaw->read(content, fSize[i]-sizeof(CAEN_DGTZ_LHEADER_t));

   RUN_INFO* hdr = (RUN_INFO*) content;
   if (run!=hdr->run_number) {
      Warning("ReadRunInfo", "unmatched run number: %d", hdr->run_number);
      run=hdr->run_number;
   }
   sub=hdr->sub_run_number;
   sec=hdr->linux_time_sec;
   nmax=hdr->custom_size*8;
   nfw=hdr->nsamples_lfw*8;
   nbw=hdr->nsamples_lbk*8;
   thr=hdr->threshold_upper;

   delete[] content;
}

//______________________________________________________________________________
//

void Reader::ReadEvent(int i)
{
   size_t evt_size=fSize[i]-sizeof(CAEN_DGTZ_LHEADER_t);
   char *content = new char[evt_size];
   fRaw->read(content, evt_size);

   unsigned int *data = (unsigned int*) (content);
   unsigned int processed=0;
   unsigned short ch=0;
   while(processed<evt_size && ch<nch) {
      ReadWF(ch,data);
      data+=data[0]; // move pointer to next raw waveform
      processed+=data[0]*4; // data size in byte
      ch++; // increase ch count
   }

   delete[] content;
}

//------------------------------------------------------------------------------

void Reader::ReadWF(unsigned short ch, unsigned int *data)
{
   WF* wf = At(ch); // must have been initialized
   if (wf->pmt.id==-1) return; // skip empty channel

   wf->Reset();
   wf->freq=1*GHz;
   wf->ctrg=data[1];

   // ped_type==0 -> integer baseline, ==1 -> float baseline
   int ped_type = (data[2]>>31)&0x1;
   // 2^(ped_N_samples+2)= number of samples used for baseline calculation
   int ped_N_samples = (data[2]>>28)&0x7; 
   int ped_sum = data[2]&0x7fff; // sum of ADC counts
   if (ped_type==0) wf->ped = data[2]; // integer
   else wf->ped = ped_sum/pow(2.,ped_N_samples+2);

   bool isBaseline=true;
   size_t nSamples=0; // total number of samples
   size_t nSuppressed=0; // number of suppressed samples
   for (unsigned int i=3;i<data[0];i++) {
      unsigned int nZippedSamples = (data[i]>>30);
      if (nZippedSamples==3) { // got 3 zipped samples
         // decode 3 samples
         wf->smpl.push_back((data[i])     & 0x3FF);
         wf->smpl.push_back((data[i]>>10) & 0x3FF);
         wf->smpl.push_back((data[i]>>20) & 0x3FF);
         // add a pulse
         if (isBaseline==true) {
            Pulse pls;
            pls.bgn=nSamples;
            pls.end=nSamples;
            wf->pls.push_back(pls);
            wf->np++;
         }
         isBaseline=false;
         nSamples += 3;
      } else if (nZippedSamples==0) { // no zipped samples
         // get number of skipped samples
         unsigned int nSkippedSamples = data[i]*8;
         if (nSkippedSamples+nSamples>nmax) {
            Warning("ReadWF",
                  "%d (skipped) + %zu (real) > %d, break!",
                  nSkippedSamples, nSamples, nmax);
            break;
         }
         // fill skipped samples with baseline
         nSuppressed+=nSkippedSamples;
         nSamples+=nSkippedSamples;
         for (unsigned int j=0; j<nSkippedSamples; j++)
            wf->smpl.push_back(wf->ped);
         // finish adding a pulse
         if (isBaseline==false) 
            wf->pls.back().end=nSamples;
         isBaseline=true;
      } else { // skip the rest samples if nZippedSamples!=0 or 3
         Warning("ReadWF",
               "number of zipped samples in PMT %d = %d, skip the rest.",
               wf->pmt.id, nZippedSamples);
         break;
      }
   }
   // check length of wf
   // sometimes nSamples == nmax + 8 (or 16) when the waveform ends with
   // non-suppressed samples. It looks like a DAQ bug.
   if (nSamples>nmax+size_t(16)) {
      Warning("ReadWF",
            "number of samples in PMT %2d is %zu,", wf->pmt.id, nSamples);
      Warning("ReadWF",
            "while total number of samples is %d", nmax);
   }
   wf->ns=nSamples;

   // update end of last pulse if the last sample is not 0-suppressed
   if (wf->np==0) return; // no unsuppressed region
   if (wf->pls.back().end==wf->pls.back().bgn)
      wf->pls.back().end=nSamples;
}

//------------------------------------------------------------------------------

void Reader::Suppress(unsigned short ch)
{ 
   Scan(ch); // scan pulses in a waveform
   WF* wf = At(ch); // must have been filled
   if (wf->pmt.id==-1) return; // skip empty channels
   // suppress samples in between pulses
   for (int i=0; i<=wf->np; i++) {
      int end=0, bgn=nmax;
      if (i!=0) end=wf->pls[i-1].end;
      if (i!=wf->np) bgn=wf->pls[i].bgn;
      for (int j=end/*prev end*/; j<bgn/*new bgn*/; j++) wf->smpl[j]=0;
   }
}

//------------------------------------------------------------------------------

void Reader::Scan(unsigned short ch)
{ 
   WF* wf = At(ch); // must have been filled
   if (wf->pmt.id==-1) return; // skip empty channels
   if (wf->np==0) return; // nothing to suppress

   // calculate pedestal; ADC -> npe
   if (run<5) Calibrate(ch, 30);
   else Calibrate(ch, 96);

   // reset pulses defined by hardware 0-suppression
   wf->np=0; // do it after calibration
   wf->pls.resize(0);
   
   bool isPed=true;
   int bgn=0, end=0; // beginning and end of a pulse
   for (int i=0; i<nmax; i++) {
      int next1 = (i<nmax-1)?(i+1):(nmax-1); // index of next sample
      int next2 = (i<nmax-2)?(i+2):(nmax-1); // index of next next sample
      if (  wf->smpl[i]>thr/wf->pmt.gain &&
            wf->smpl[next1]>thr/wf->pmt.gain &&
            wf->smpl[next2]>thr/wf->pmt.gain) { // above threshold
         if (isPed==false) continue; // previous sample also above threshold
         if (i-nbw<=end) continue; // overlap with previous pulse

         // update previous pulse if any
         for (int j=bgn; j<end; j++) {
            if (wf->smpl[j]>wf->pls.back().h) {
               wf->pls.back().h=wf->smpl[j];
               wf->pls.back().ih=j;
            }
            if (wf->smpl[j]==wf->ped/wf->pmt.gain)
               wf->pls.back().SetBit(Pulse::kSaturated);
            wf->pls.back().npe+=wf->smpl[j];
         }
         if (end!=bgn) wf->pls.back().end=end;

         // create a new pulse
         Pulse pls;
         pls.ithr=i;
         pls.bgn=i-nbw;
         wf->pls.push_back(pls);

         bgn=i-nbw;
         isPed=false; // flip flag
      } else { // below threshold
         if (isPed) continue; // previous samples also below threshold
         end=i+nfw;
         isPed=true; // flip flag
      }
   }
   if (end>=nmax) end=nmax;
   // if the last sample != pedestal, end needs to be updated
   if (isPed==false) end=nmax;

   // update last pulse
   for (int j=bgn; j<end; j++) {
      if (wf->smpl[j]>wf->pls.back().h) {
         wf->pls.back().h=wf->smpl[j];
         wf->pls.back().ih=j;
      }
      if (wf->smpl[j]==wf->ped/wf->pmt.gain)
         wf->pls.back().SetBit(Pulse::kSaturated);
      wf->pls.back().npe+=wf->smpl[j];
   }
   if (end!=bgn) wf->pls.back().end=end;
   wf->np = wf->pls.size();
}

//------------------------------------------------------------------------------

void Reader::Calibrate(unsigned short ch, unsigned short nSamples)
{ 
   WF* wf = At(ch); // must have been filled
   if (wf->pmt.id==-1) return; // skip empty channel
   if (wf->np==0) return; // nothing to calibrate

   if (wf->TestBit(WF::kCalibrated)) return; // already done
   if (nfw<nSamples) return; // samples not enough

   // rough calculation of pedestal
   double rough=0; // rough value of pedestal
   for (int i=wf->pls[0].bgn; i<wf->pls[0].bgn+nSamples; i++)
      rough += wf->smpl[i]/nSamples;

   // precise calculation of pedestal
   nSamples = 0; // re-count # of samples useful for pedestal calculation
   int notUseful = 0; // flag to skip samples far away from rough value
   wf->ped=0; double ped2=0; // pedestal and squared
   double prev = 0, prev2 = 0; // previous sample and squared
   for (int i=wf->pls[0].bgn; i<wf->pls[0].bgn+nfw; i++) {
      if(notUseful==1){ // previous sample not useful
         if((rough-wf->smpl[i])<2){ // current sample useful
            wf->ped += wf->smpl[i];
            ped2 += wf->smpl[i]*wf->smpl[i];
            nSamples++;
            notUseful = 0;
         }else{ // current sample also not useful
            wf->ped = wf->ped - prev;
            ped2 = ped2 - prev2;
            nSamples = nSamples - 1;
            notUseful = 2;
         }
      }else if(notUseful==2){ // 2 previous samples not useful
         if((rough-wf->smpl[i])<2){ // current sample useful
            wf->ped += wf->smpl[i];
            ped2 += wf->smpl[i]*wf->smpl[i];
            nSamples++;
            notUseful = 0;
         }
      }else{ // useful sample
         wf->ped += wf->smpl[i];
         ped2 += wf->smpl[i]*wf->smpl[i];
         nSamples++;
         if(abs(rough-wf->smpl[i])>2){
            notUseful=1;
            prev = wf->smpl[i];
            prev2 = wf->smpl[i]*wf->smpl[i];
         }
      }
   }
   wf->ped = wf->ped/nSamples;
   ped2 = ped2/nSamples;
   wf->prms = sqrt(ped2 - wf->ped*wf->ped);

   // remove pedestal, flip waveform, ADC to npe
   for (int i=0; i<nmax; i++) {
      wf->smpl[i] = (wf->ped - wf->smpl[i])/wf->pmt.gain;
   }
   wf->smpl.resize(nmax); // remove unnecessary samples

   wf->SetBit(WF::kCalibrated);
}
