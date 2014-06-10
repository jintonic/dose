#include <cmath>
using namespace std;

#include <WF.h>
using namespace NICE;

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
   LoadIndex();
   if (fBegin.size()>0) return;

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

      evt = hdr.eventCount;
      cnt = hdr.TTimeTag;
      
      if (hdr.reserved2>1) {
         Warning("GetEntry", "unknown entry %d, skipped", i);
         fRaw->seekg(fSize[i]-sizeof(CAEN_DGTZ_LHEADER_t),ios::cur);
         return;
      }
      if (hdr.reserved2==1) ReadRunInfo(i);
      else if (hdr.reserved2==0) ReadEvent(i);
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
   // read data
   size_t evt_size=fSize[i]-sizeof(CAEN_DGTZ_LHEADER_t);
   char *content = new char[evt_size];
   fRaw->read(content, evt_size);

   // decode data
   UInt_t *data = (UInt_t*) (content);
   unsigned int processed=0;
   unsigned short channel=0;
   while(processed<evt_size && channel<nch) {
      processed+=data[0]*4; // data size in byte

      WF* wf = At(channel); // must have been initialized
      wf->samples.resize(0); // reset samples
      wf->ctrg = data[1]; // trigger time for this channel

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
      size_t nPulses=0; // number of not suppressed regions
      for (unsigned int i=3;i<data[0];i++) {
         unsigned int nZippedSamples = (data[i]>>30);
         if (nZippedSamples==3) { // got 3 zipped samples
            // decode 3 samples
            wf->samples.push_back((data[i])     & 0x3FF);
            wf->samples.push_back((data[i]>>10) & 0x3FF);
            wf->samples.push_back((data[i]>>20) & 0x3FF);
            // add a pulse
            if (isBaseline==true) {
               Pulse pls;
               pls.bgn=nSamples;
               pls.end=nSamples;
               wf->pulses.push_back(pls);
               nPulses++;
            }
            isBaseline=false;
            nSamples += 3;
         } else if (nZippedSamples==0) { // no zipped samples
            // get number of skipped samples
            unsigned int nSkippedSamples = data[i]*8;
            if (nSkippedSamples+nSamples>nmax) {
               Warning("ReadEvent",
                     "%d (skipped) + %zu (real) > %d, break!",
                     nSkippedSamples, nSamples, nmax);
               break;
            }
            // fill skipped samples with baseline
            nSuppressed+=nSkippedSamples;
            nSamples+=nSkippedSamples;
            for (unsigned int j=0; j<nSkippedSamples; j++)
               wf->samples.push_back(wf->ped);
            // finish adding a pulse
            if (isBaseline==kFALSE) 
               wf->pulses.back().end=nSamples;
            isBaseline=true;
         } else { // skip the rest samples if nZippedSamples!=0 or 3
            Warning("ReadEvent",
                  "number of zipped samples in PMT %d = %d, skip the rest.",
                  wf->pmt.id, nZippedSamples);
            break;
         }
      }
      // check length of wf
      if (nSamples!=nmax) {
         Warning("ReadEvent",
               "number of samples in PMT %2d is %zu,", wf->pmt.id, nSamples);
         Warning("ReadEvent",
               "while total number of samples is %d", nmax);
      }
      // update end of last pulse if the last sample is not 0-suppressed
      data+=data[0]; // move pointer to next raw waveform
      channel++; // increase channel count

      // re-calculate baseline for runs with integer hardware baseline
      if (nPulses==0) continue; // no unsuppressed region
      if (wf->pulses.back().end==wf->pulses.back().bgn)
         wf->pulses.back().end=nSamples;
      if (nSuppressed==nSamples) continue; // all samples are suppressed
      if (ped_type==1) continue; // already float baseline
      if (nfw<30) {
         Warning("ReadEvent", "no enough samples to calculate baseline");
         continue;
      }
      short nBaselineSamples = 30;

      double baseline=0;
      for (int i=wf->pulses[0].bgn; i<wf->pulses[0].bgn+nBaselineSamples; i++)
         baseline += wf->samples[i]/nBaselineSamples;

      double tmpBaseline = baseline, baseline2=0;
      nBaselineSamples = 0;
      baseline = 0.;
      int baseline_calc_flag = 0;
      double sample_prev = 0; double sample2_prev = 0;
      for (int i=wf->pulses[0].bgn; i<wf->pulses[0].bgn+nfw; i++) {
         if(baseline_calc_flag==1){
            if((tmpBaseline-wf->samples[i])<2){
               baseline += wf->samples[i];
               baseline2 += wf->samples[i]*wf->samples[i];
               nBaselineSamples++;
               baseline_calc_flag = 0;
            }else{
               baseline  = baseline - sample_prev;
               baseline2  = baseline2 - sample2_prev;
               nBaselineSamples = nBaselineSamples - 1;
               baseline_calc_flag = 2;
            }
         }else if(baseline_calc_flag==2){
            if((tmpBaseline-wf->samples[i])<2){
               baseline += wf->samples[i];
               baseline2 += wf->samples[i]*wf->samples[i];
               nBaselineSamples++;
               baseline_calc_flag = 0;
            }
         }else{
            baseline += wf->samples[i];
            baseline2 += wf->samples[i]*wf->samples[i];
            nBaselineSamples++;
            if(abs(tmpBaseline-wf->samples[i])>2){
               baseline_calc_flag=1;
               sample_prev = wf->samples[i];
               sample2_prev = wf->samples[i]*wf->samples[i];
            }
         }
      }
      baseline = baseline/nBaselineSamples;
      baseline2 = baseline2/nBaselineSamples;
      wf->prms = sqrt(baseline2 - baseline*baseline);
      wf->ped = baseline;
   }

   delete[] content;
}

