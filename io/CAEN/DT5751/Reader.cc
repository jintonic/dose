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
   nch=4;
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
   // open data file and put the "get" pointer to the end of the file
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
   if (fileSize<sizeof(EVENT_HEADER_t)) {
      Error("Index", "%s/%s is too small, return", fPath.Data(), fName.Data()); 
      fRaw->close();
      return;
   }

   LoadIndex();
   if (fBegin.size()>0) return;

   // move "get" pointer back to the beginning to start reading
   fRaw->seekg(0, ios::beg);

   // get total number of events and
   // save the starting points and sizes of events
   fBegin.resize(0);
   fSize.resize(0);
   while (fRaw->good() && size_t(fRaw->tellg())<fileSize) {
      //save starting points
      fBegin.push_back(fRaw->tellg());

      // save event size
      EVENT_HEADER_t hdr;
      fRaw->read(reinterpret_cast<char *> (&hdr), sizeof(EVENT_HEADER_t));
      if (hdr.size<=sizeof(EVENT_HEADER_t)) {
         fSize.push_back(hdr.size);
         Error("Index", "entry %zu is too small, stop indexing", fBegin.size());
         break;
      }
      fSize.push_back(hdr.size);

      //skip the rest of the block
      fRaw->seekg(hdr.size-sizeof(EVENT_HEADER_t), ios::cur);
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
      EVENT_HEADER_t hdr;
      fRaw->read(reinterpret_cast<char *> (&hdr), sizeof(EVENT_HEADER_t));

      if (hdr.type==0) {
         ReadRunCfg(i);
      } else {
         evt = hdr.evtCnt;
         cnt = hdr.trgCnt;
         ReadEvent(i);
      }
   }
}

//------------------------------------------------------------------------------

void Reader::ReadRunCfg(int i)
{
   RUN_CFG_t cfg;
   fRaw->read(reinterpret_cast<char *> (&cfg), sizeof(RUN_CFG_t));

   if ((uint32_t)run!=cfg.run) {
      Warning("ReadRunCfg", "unmatched run number: %d", cfg.run);
      run=cfg.run;
   }
   sub=cfg.subrun;
   sec=cfg.tsec;
   nmax=cfg.ns;
   nfw=100; // set fw smpl not to be suppressed
   nbw=100; // set bw smpl not to be suppressed
   thr=3;   // set threshold for software zero suppression
}

//______________________________________________________________________________
//

void Reader::ReadEvent(int i)
{
   size_t evt_size=fSize[i]-sizeof(EVENT_HEADER_t);
   char *content = new char[evt_size];
   fRaw->read(content, evt_size);

   uint16_t *data = (uint16_t*) (content);
   unsigned int processed=0;
   unsigned short ch=0;
   while(processed<evt_size && ch<nch) {
      ReadWF(ch,data);
      data+=nmax*sizeof(uint16_t); // move pointer to next raw waveform
      processed+=nmax*sizeof(uint16_t); // data size in byte
      ch++; // increase ch count
   }

   delete[] content;
}

//------------------------------------------------------------------------------

void Reader::ReadWF(unsigned short ch, uint16_t *data)
{
   WF* wf = At(ch); // must have been initialized
   if (wf->pmt.id==-1) return; // skip empty channel

   wf->Reset();
   wf->freq=1*GHz;

   for (int i=0; i<nmax; i++) wf->smpl.push_back(data[i]);
}

//------------------------------------------------------------------------------

void Reader::Suppress(unsigned short ch)
{ 
   WF* wf = At(ch); // must have been filled
   if (wf->pmt.id==-1) return; // skip empty channels
   if (wf->smpl.size()<96) return; // samples not enough

   // calculate pedestal; ADC -> npe
   Calibrate(ch, 96);

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

         // 0-suppress samples between pulses
         bgn=i-nbw;
         for (int j=end/*prev end*/; j<bgn/*new bgn*/; j++) wf->smpl[j]=0;
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
 
   // 0-suppress samples from the last pulse to the end of waveform
   for (int j=end; j<nmax; j++) wf->smpl[j]=0;
}

//------------------------------------------------------------------------------

void Reader::Calibrate(unsigned short ch, unsigned short nSamples)
{ 
   WF* wf = At(ch); // must have been filled
   if (wf->pmt.id==-1) return; // skip empty channel

   if (wf->TestBit(WF::kCalibrated)) return; // already done
   if (wf->smpl.size()<nSamples) return; // samples not enough

   // rough calculation of pedestal
   double rough=0; // rough value of pedestal
   for (int i=0; i<nSamples; i++) rough += wf->smpl[i]/nSamples;

   // precise calculation of pedestal
   nSamples = 0; // re-count # of samples useful for pedestal calculation
   int notUseful = 0; // flag to skip samples far away from rough value
   wf->ped=0; double ped2=0; // pedestal and squared
   double prev = 0, prev2 = 0; // previous sample and squared
   for (int i=0; i<100; i++) {
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

   wf->SetBit(WF::kCalibrated);
}
