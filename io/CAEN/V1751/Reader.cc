#include <cmath>
using namespace std;

#include <Units.h>
using namespace UNIC;

#include "Reader.h"
using namespace NICE;

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
   int evt_size=fSize[i]-sizeof(CAEN_DGTZ_LHEADER_t);
   char *content = new char[evt_size];
   fRaw->read(content, evt_size);

   char *ptr = content;
   int processed=0, channel=0;
   while(processed<evt_size) {
      if (channel>=nch) {
         Warning("ReadEvent", "ch. %d >= %d (max), skipped", channel, nch);
         continue;
      }
      WF* wf = (WF*) At(channel);

      // decode waveform data
      unsigned int *data = (unsigned int*) (ptr);

      int data_size = data[0]*4; // data size in byte
      ptr+=data_size; // move pointer to next raw waveform
      processed+=data_size;

      wf->ttrg = data[1]*8*ns; // trigger time for this channel

      // baselineFlag==0 -> integer baseline, ==1 -> float baseline
      int baselineFlag = (data[2]>>31)&0x1;
      // 2^(baselineNsam+2)= number of samples used for baseline calculation
      int baselineNsam = (data[2]>>28)&0x7; 
      int baselineSum = data[2]&0x7ffff; // sum of ADC counts
      if (baselineFlag==0) wf->ped = data[2]; // integer
      else wf->ped = baselineSum/pow(2.,baselineNsam+2);

      Bool_t isBaseline=kTRUE;
      size_t nSamples=0; // total number of samples
      size_t nSuppressed=0; // number of suppressed samples
      for (int i=3;i<data_size/4;i++) {
         int nZippedSamples = (data[i]>>30);
         if (nZippedSamples==3) { // got 3 zipped samples
            // decode 3 samples
            wf->samples.push_back((data[i])     & 0x000003FF);
            wf->samples.push_back((data[i]>>10) & 0x000003FF);
            wf->samples.push_back((data[i]>>20) & 0x000003FF);

            if (isBaseline==kTRUE) {
               Pulse pls;
               pls.bgn=nSamples;
               pls.end=nSamples;
               wf->pulses.push_back(pls);
            }
            isBaseline=kFALSE;

            nSamples += 3;
         } else if (nZippedSamples==0) { // no zipped samples
            if (isBaseline==kFALSE)
               wf->pulses.back().end=nSamples;
            isBaseline=kTRUE;

            // get number of skipped samples
            int nSkippedSamples = data[i]*8;
            if (nSkippedSamples+nSamples>=12000) {
               Warning("ReadEvent",
                     "number of skipped samples >=12000, break!");
               break;
            }
            // fill skipped samples with baseline
            nSuppressed+=nSkippedSamples;
            for (int j=0; j<nSkippedSamples; j++) {
               wf->samples.push_back(wf->ped);
               nSamples++;
            }
         } else { // skip the rest samples if nZippedSamples!=0 or 3
            Warning("ReadEvent",
                  "number of zipped samples in PMT %d = %d, skip the rest.",
                  wf->id, nZippedSamples);
            break;
         }
      }
      // if all samples are suppressed, skip this wf
      if (nSuppressed==nSamples) {
         channel++;
         continue;
      }
      // check length of wf
      if (nSamples!=nmax) {
         Warning("ReadEvent",
               "number of samples in PMT %2d (ch.%d) is %zu,",
               wf->id, channel, nSamples);
         Warning("ReadEvent",
               "while total number of samples is %zu", nmax);
      }
      // update end of last pulse
      if (wf->pulses.back().end==wf->pulses.back().bgn)
         wf->pulses.back().end=nSamples;

      // re-calculate baseline for runs with integer hardware baseline
      if (baselineFlag==0) { // integer baseline
         // baseline calculation
         double realBaseline=0.; double realBaselineSquare=0.;
         size_t nBaselineSamples_max=96, nBaselineSamples = 0;
         if(nSamples<nBaselineSamples_max) nBaselineSamples_max = nSamples;
         for (int i=wf->pulses[0].bgn; i<int(wf->pulses[0].bgn+nBaselineSamples_max); i++) {
            realBaseline += wf->samples[i];
            realBaselineSquare += wf->samples[i]*wf->samples[i];
            nBaselineSamples++;
         }
         realBaseline = realBaseline/nBaselineSamples;
         realBaselineSquare = realBaselineSquare/nBaselineSamples;
         wf->prms = sqrt(realBaselineSquare - realBaseline*realBaseline);

         double realBaseline_tmp = realBaseline;
         nBaselineSamples = 0;
         realBaseline = 0.; realBaselineSquare = 0.;
         int baseline_calc_flag = 0;
         double sample_prev = 0; double sampleSquare_prev = 0;
         for (int i=wf->pulses[0].bgn; i<int(wf->pulses[0].bgn+nBaselineSamples_max); i++) {
            if(baseline_calc_flag==1){
               if((realBaseline_tmp-wf->samples[i])<2){
                  realBaseline += wf->samples[i];
                  realBaselineSquare += wf->samples[i]*wf->samples[i];
                  nBaselineSamples++;
                  baseline_calc_flag = 0;
               }else{
                  realBaseline  = realBaseline - sample_prev;
                  realBaselineSquare  = realBaselineSquare - sampleSquare_prev;
                  nBaselineSamples = nBaselineSamples - 1;
                  baseline_calc_flag = 2;
               }
            }else if(baseline_calc_flag==2){
               if((realBaseline_tmp-wf->samples[i])<2){
                  realBaseline += wf->samples[i];
                  realBaselineSquare += wf->samples[i]*wf->samples[i];
                  nBaselineSamples++;
                  baseline_calc_flag = 0;
               }
            }else{
               realBaseline += wf->samples[i];
               realBaselineSquare += wf->samples[i]*wf->samples[i];
               nBaselineSamples++;
               if((realBaseline_tmp-wf->samples[i])>2){
                  baseline_calc_flag=1;
                  sample_prev = wf->samples[i];
                  sampleSquare_prev = wf->samples[i]*wf->samples[i];
               }
            }
         }
         realBaseline = realBaseline/nBaselineSamples;
         realBaselineSquare = realBaselineSquare/nBaselineSamples;
         wf->prms = sqrt(realBaselineSquare - realBaseline*realBaseline);

         wf->ped = realBaseline;
      }
      channel++;
   }

   delete[] content;
}

