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
      Info("LoadIndex","cannot open %s, return",name);
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
   fRaw = new ifstream(fName.Data(), ios::in | ios::binary | ios::ate); 
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

   // get total number of events and save the starting points and sizes of
   // events into vectors for later use
   fBegin.resize(0);
   fSize.resize(0);
   while (fRaw->good() && size_t(fRaw->tellg())<fileSize) {
      //save starting points
      fBegin.push_back(fRaw->tellg());

      // save event size
      CAEN_DGTZ_LHEADER_t hdr;
      fRaw->read(reinterpret_cast<char *> (&hdr), sizeof(CAEN_DGTZ_LHEADER_t));
      if (hdr.evtSize*4<sizeof(CAEN_DGTZ_LHEADER_t)) {
         Error("Index", "entry %d is too small, stop indexing", fBegin.size());
         break;
      }
      fSize.push_back(hdr.evtSize*4);
      
      //skip the rest of the block
      fRaw->seekg(hdr.evtSize, ios::cur);
   }
}

//------------------------------------------------------------------------------

void Reader::DumpIndex()
{
   for (size_t i=0; i<fBegin.size(); i++)
      Printf("%20zu %zu", fBegin[i], fSize[i]);
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
      if (hdr.reserved2==0) ReadRunInfo(i);
      else if (hdr.reserved2==1) ReadEvent(i);
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
   char *content = new char[fSize[i]-sizeof(CAEN_DGTZ_LHEADER_t)];
   fRaw->read(content, fSize[i]-sizeof(CAEN_DGTZ_LHEADER_t));

   size_t nwf=0;
   char *ptr = content;
   size_t processed=0;
   while(processed<fSize[i]-sizeof(CAEN_DGTZ_LHEADER_t)) {
      short cable   = *((short*)ptr);
      short crate   = cable>>10 & 0x0000000F;
      short module  = cable>>5  & 0x0000001F;
      short channel = cable     & 0x0000000F;
      channel += crate*21*8+module*8;
      if (channel>=nch) {
         Warning("ReadEvent", "ch. %d >= %d (max), skipped", channel, nch);
         continue;
      }
      WF* wf = (WF*) At(channel);

      // get data format, it is always 0
      short format = *((short*) (ptr+=2));

      // get data size in unit of byte
      int size = *((int*) (ptr+=2));
      if (size>1.8*1024*1024) {
         Warning("ReadEvent", "size of WF exceeds 1.8 MB. Break!");
         break;
      }

      // decode waveform data
      int *data = (int*) (ptr+=4);
      ptr+=size; // move pointer to next raw waveform
      int dataSize = data[0]; // data size in unit of integer
      double triggerTime = data[1]*8*ns; // trigger time for this channel

      // baselineFlag==0 -> integer baseline, ==1 -> float baseline
      int baselineFlag = (data[2]>>31)&0x1;
      // 2^(baselineNsam+2)= number of samples used for baseline calculation
      int baselineNsam = (data[2]>>28)&0x7; 
      int baselineSum = data[2]&0x7ffff; // sum of ADC counts
      double baseline; // baseline for this channel
      if (baselineFlag==0) baseline = data[2]; // integer
      else baseline = baselineSum/pow(2.,baselineNsam+2);

      Bool_t isBaseline=kTRUE;
      int nSamples=0; // total number of samples
      int nSuppressed=0; // number of suppressed samples
      double samples[12000]={0}; // array to save samples
      for (int i=3;i<dataSize;i++) {
         int nZippedSamples = (data[i]>>30);
         if (nZippedSamples==3) { // got 3 zipped samples
            // decode 3 samples
            samples[nSamples]   = (data[i])     & 0x000003FF;
            samples[nSamples+1] = (data[i]>>10) & 0x000003FF;
            samples[nSamples+2] = (data[i]>>20) & 0x000003FF;

            // record peak start time
            if (isBaseline==kTRUE) {
               if (npeaks>=200) {
                  Warning("ReadEvent", "%d peaks detected,",npeaks);
                  Warning("ReadEvent", "only 200 peaks can be handled,");
                  Warning("ReadEvent", "the rest peaks won't be recorded!");
               } else {
                  tpstart[npeaks]=nSamples;
                  tpend[npeaks]=tpstart[npeaks];
                  npeaks++;
               }
            }
            isBaseline=kFALSE;

            // increase number of samples
            nSamples += 3;
         } else if (nZippedSamples==0) { // no zipped samples
            if (isBaseline==kFALSE) {
               if (npeaks>0 && npeaks<200) {
                  tpend[npeaks-1]=nSamples;
               }
            }
            isBaseline=kTRUE;

            // for runs taken with 0-suppression firmware,
            // but without turning on 0-suppression,
            // nZippedSamples should never equal to 0.
            // However, due to an FADC software bug, 
            // see xmass DAQ meeting minutes, hiraide-2012-0406-FADC.pptx,
            // sometimes we fall into this condition.
            // We have to skip the problematic samples.
            if (fEvent->GetFADCHeader()->GetRun()>5950 &&
                  fEvent->GetFADCHeader()->GetRun()<6642) {
               fEvent->GetHeader()->SetStatus(XEventHeader::kFADCProblem);
               break;
            }
            // get number of skipped samples
            int nSkippedSamples = data[i]*8;
            if (nSkippedSamples+nSamples>=12000) {
               Warning("ReadEvent",
                     "number of skipped samples >=12000, break!");
               fEvent->GetHeader()->SetStatus(XEventHeader::kFADCProblem);
               break;
            }
            // fill skipped samples with baseline
            nSuppressed+=nSkippedSamples;
            for (int j=0; j<nSkippedSamples; j++) {
               samples[nSamples]=baseline;
               nSamples++;
            }
         } else { // skip the rest samples if nZippedSamples!=0 or 3
            fNZippedWarning++;
            if (fNZippedWarning==100) Warning("ReadEvent",
                  "zipped sample warnings exceed 100 lines!");
            else if (fNZippedWarning<100) Warning("ReadEvent",
                  "number of zipped samples in PMT %d = %d, skip the rest.",
                  pmtId, nZippedSamples);
            status=XWaveform::kHasFADCProblem;
            fEvent->GetHeader()->SetStatus(XEventHeader::kFADCProblem);
            break;
         }
      }
      // if all samples are suppressed, skip this wf
      if (nSuppressed==nSamples) continue;
      // if more than 0 samples are suppressed, 0-suppression works
      if (nSuppressed>0) format=1; 
      // check length of wf
      double totalWindow=fEvent->GetFADCHeader()->GetTotalTimeWindow();
      if (pmtId>0 && nSamples*ns!=totalWindow && GetVerbosity()>6) {
         Warning("ReadEvent",
               "number of samples in PMT %3d (cr.%d-mo.%d-ch.%d) is %d,",
               pmtId, crate, module, channel, nSamples);
         Warning("ReadEvent",
               "while total number of samples is %4.0f", totalWindow);
         status=XWaveform::kHasFADCProblem;
         fEvent->GetHeader()->SetStatus(XEventHeader::kFADCProblem);
      }
      // print debug information
      if (GetVerbosity()>5) {
         cout<<"fadc wf ["<<iwf<<"]----------------------------------"<<endl;
         cout<<"fadcbuf.fadc["<<iwf<<"].crate:   "<<crate<<endl;
         cout<<"fadcbuf.fadc["<<iwf<<"].module:  "<<module<<endl;
         cout<<"fadcbuf.fadc["<<iwf<<"].channel: "<<channel<<endl;
         cout<<"fadcbuf.fadc["<<iwf<<"].PMT_Id: "<<pmtId<<endl;
         cout<<"fadcbuf.fadc["<<iwf<<"].data_format: "<<format<<endl;
         cout<<"fadcbuf.fadc["<<iwf<<"].data_size: "<<size<<endl;
         cout<<"number of samples: "<<nSamples<<endl;
      }

      // update end time of last peak
      if (tpend[npeaks-1]==tpstart[npeaks-1]) tpend[npeaks-1]=nSamples;

      // software 0-suppression for runs without hardware 0-suppression;
      // re-calculate baseline for runs with integer hardware baseline

      // do it only for good PMTs
      if (status!=XWaveform::kInGoodPMT) {
         XWaveform aWaveform(nSamples,samples,cable,pmtId,status,format);
         wfs->AddWF(aWaveform);
         continue;
      }


      // fill raw wf summary
      rawSum.Reset(); rawSum.SetPMTId(pmtId); rawSum.SetWFId(iwf);
      rawSum.SetBaseline(baseline); // remember hardware baseline
      rawSum.SetTriggerTime(triggerTime);
      rawSum.SetPMTStatus((XWFSummary::EPMTStatus) status);

      // fill wf summary for 0-suppression
      summary.Reset(); summary.SetPMTId(pmtId); summary.SetWFId(iwf);
      summary.SetBaseline(baseline); // remember hardware baseline
      summary.SetTriggerTime(triggerTime);
      summary.SetPMTStatus((XWFSummary::EPMTStatus) status);

      // The baseline given by hardware was once an integer (eg. 901.6 is
      // rounded to 901). The real baseline is calculated using the first 30
      // real samples.
      if (baselineFlag==0) { // integer baseline
         //        std::cout <<"kenkou-0" << std::endl;
         // baseline calculation
         double realBaseline=0.; double realBaselineSquare=0.;
         double baselineRMS=0;  
         short nBaselineSamples_max=96; short nBaselineSamples = 0;
         if(nSamples<nBaselineSamples_max) nBaselineSamples_max = nSamples;
         for (int i=tpstart[0]; i<(tpstart[0]+nBaselineSamples_max); i++) {
            realBaseline += (double)(samples[i]);
            realBaselineSquare += (double)(samples[i]*samples[i]);
            nBaselineSamples++;
         }
         realBaseline = realBaseline/(double)(nBaselineSamples);
         realBaselineSquare = realBaselineSquare/(double)(nBaselineSamples);
         baselineRMS = sqrt(realBaselineSquare - realBaseline*realBaseline);

         double realBaseline_tmp = realBaseline;
         nBaselineSamples = 0;
         realBaseline = 0.; realBaselineSquare = 0.; baselineRMS = 0.;
         int baseline_calc_flag = 0;
         double sample_prev = 0; double sampleSquare_prev = 0;
         for (int i=tpstart[0]; i<(tpstart[0]+nBaselineSamples_max); i++) {
            if(baseline_calc_flag==1){
               if((realBaseline_tmp-samples[i])<2){
                  realBaseline += (double)(samples[i]);
                  realBaselineSquare += (double)(samples[i]*samples[i]);
                  nBaselineSamples++;
                  baseline_calc_flag = 0;
               }else{
                  realBaseline  = realBaseline - sample_prev;
                  realBaselineSquare  = realBaselineSquare - sampleSquare_prev;
                  nBaselineSamples = nBaselineSamples - 1;
                  baseline_calc_flag = 2;
               }
            }else if(baseline_calc_flag==2){
               if((realBaseline_tmp-samples[i])<2){
                  realBaseline += (double)(samples[i]);
                  realBaselineSquare += (double)(samples[i]*samples[i]);
                  nBaselineSamples++;
                  baseline_calc_flag = 0;
               }
            }else{
               realBaseline += (double)(samples[i]);
               realBaselineSquare += (double)(samples[i]*samples[i]);
               nBaselineSamples++;
               if((realBaseline_tmp-samples[i])>2){
                  baseline_calc_flag=1;
                  sample_prev = (double)samples[i];
                  sampleSquare_prev = (double)(samples[i]*samples[i]);
               }
            }
         }
         realBaseline = realBaseline/(double)(nBaselineSamples);
         realBaselineSquare = realBaselineSquare/(double)(nBaselineSamples);
         baselineRMS = sqrt(realBaselineSquare - realBaseline*realBaseline);

         baseline = realBaseline;
         summary.SetPedestal(realBaseline);
         summary.SetPedestalRMS(baselineRMS);
         rawSum.SetPedestal(realBaseline);
         rawSum.SetPedestalRMS(baselineRMS);
      }

      // loop over samples
      isBaseline=kTRUE;
      double nPEs=0, peakHeight=9999, peakNegativeHeight=-9999,tAtSummit=0;
      double maxWidth=0, tmpWidth=0;
      int maxWidth_flag = 0; 
      double tRise=0, tFall=0;
      double tStart=0, tThresholdUp=0, tThresholdDn=0, tEnd=0;
      fEvent->GetFADCHeader()->SetBackwardTimeWindow(32*ns);
      double forwardWindow=fEvent->GetFADCHeader()->GetForwardTimeWindow();
      double backwardWindow=fEvent->GetFADCHeader()->GetBackwardTimeWindow();
      XWFSummary::EWFStatus peakStatus=XWFSummary::kIsOK;

      int iNext, iNextNext;
      for (int i=0; i<nSamples; i++) {
         iNext = ((i<nSamples-1)?(i+1):(nSamples-1));
         iNextNext = ((i<nSamples-2)?(i+2):(nSamples-1));
         if ( baseline-samples[i]>3 &&
               baseline-samples[iNext]>3 &&
               baseline-samples[iNextNext]>3 ) { // above threshold
            if (isBaseline==kTRUE) { // previous sample is below threshold
               // if not overlapped with previous wf
               if (i*ns-backwardWindow>tEnd) { 
                  // calculate nPEs under previous peak
                  for (int j=static_cast<int>(tStart); // previous tStart
                        j<static_cast<int>(tEnd); // previous tEnd
                        j++) {
                     if (samples[j]<peakHeight) {
                        peakHeight=samples[j];
                        tAtSummit=j*ns;
                     }
                     if (samples[j]>peakNegativeHeight) {
                        peakNegativeHeight=samples[j];
                     }
                     if (samples[j]==0) peakStatus=XWFSummary::kIsSaturated;
                     nPEs+=baseline-samples[j];

                     //-------------------max Width calculation
                     if(samples[j]<(baseline-1.)){
                        if(maxWidth_flag==1){
                           tmpWidth++;
                           if(tmpWidth>maxWidth){
                              maxWidth = tmpWidth;
                           }
                        }else{
                           if(tmpWidth>maxWidth){
                              maxWidth = tmpWidth;
                           }
                           maxWidth_flag = 1;
                           tmpWidth = 1;
                        }
                     }else if(samples[j]>(baseline+1.)){
                        if(maxWidth_flag==-1){
                           tmpWidth++;
                           if(tmpWidth>maxWidth){
                              maxWidth = tmpWidth;
                           }
                        }else{
                           if(tmpWidth>maxWidth){
                              maxWidth = tmpWidth;
                           }
                           maxWidth_flag = -1;
                           tmpWidth = 1;
                        }
                     }else{
                        if(maxWidth_flag==-1&&tmpWidth>maxWidth){
                           maxWidth = tmpWidth;
                        }
                        if(maxWidth_flag==1&&tmpWidth>maxWidth){
                           maxWidth = tmpWidth;
                        }
                        tmpWidth = 0;
                        maxWidth_flag=0;
                     }
                     //-------------------end maxWidth calculation
                  }
                  nPEs/=gain;
                  peakHeight=(baseline-peakHeight)/gain;
                  peakNegativeHeight=fabs((baseline-peakNegativeHeight)/gain);

                  // add previous peak
                  if (tEnd!=0)
                     summary.AddPeak(nPEs,tThresholdUp-dT-dT_trigger,
                           tStart-dT-dT_trigger,tEnd-dT-dT_trigger,
                           tRise,tFall,tAtSummit-dT-dT_trigger,peakHeight,
                           peakNegativeHeight,maxWidth,peakStatus);

                  // start of a new peak
                  nPEs=0;
                  tThresholdUp=i*ns;
                  tStart=tThresholdUp-backwardWindow;
                  tAtSummit=tThresholdUp;
                  peakHeight=9999;
                  peakNegativeHeight=-9999;
                  peakStatus=XWFSummary::kIsOK;
                  maxWidth = 0;
                  tmpWidth = 0;
                  maxWidth_flag = 0;

                  if (fSoftwareZLE) {
                     // 0-suppress samples between peaks
                     for (int j=static_cast<int>(tEnd); // previous tEnd
                           j<static_cast<int>(tStart); // new tStart
                           j++) samples[j]=baseline;
                  }
               }
               isBaseline=kFALSE; // flip flag
            }
         } else { // below threshold
            if (isBaseline==kFALSE) { // previous samples is above threshold
               tThresholdDn=i*ns;
               tEnd=tThresholdDn+forwardWindow;
               if((int)(tEnd-tStart)%2==1) tEnd +=1.;
               isBaseline=kTRUE; // flip flag
            }
         }
      }
      // if the last peak does not go back to baseline, tEnd needs to be updated
      if (isBaseline==kFALSE) tEnd=nSamples*ns;

      // calculate nPEs under the last peak
      if (tEnd>nSamples*ns) tEnd=nSamples*ns;
      for (int j=static_cast<int>(tStart);
            j<static_cast<int>(tEnd);
            j++) {
         if (samples[j]<peakHeight) {
            peakHeight=samples[j];
            tAtSummit=j*ns;
         }
         if (samples[j]>peakNegativeHeight) {
            peakNegativeHeight=samples[j];
         }
         if (samples[j]==0) peakStatus=XWFSummary::kIsSaturated;
         nPEs+=baseline-samples[j];

         //-------------------max Width calculation
         if(samples[j]<(baseline-1.)){
            if(maxWidth_flag==1){
               tmpWidth++;
               if(tmpWidth>maxWidth){
                  maxWidth = tmpWidth;
               }
            }else{
               if(tmpWidth>maxWidth){
                  maxWidth = tmpWidth;
               }
               maxWidth_flag = 1;
               tmpWidth = 1;
            }
         }else if(samples[j]>(baseline+1.)){
            if(maxWidth_flag==-1){
               tmpWidth++;
               if(tmpWidth>maxWidth){
                  maxWidth = tmpWidth;
               }
            }else{
               if(tmpWidth>maxWidth){
                  maxWidth = tmpWidth;
               }
               maxWidth_flag = -1;
               tmpWidth = 1;
            }
         }else{
            if(maxWidth_flag==-1&&tmpWidth>maxWidth){
               maxWidth = tmpWidth;
            }
            if(maxWidth_flag==1&&tmpWidth>maxWidth){
               maxWidth = tmpWidth;
            }
            tmpWidth = 0;
            maxWidth_flag=0;
         }
         //-------------------end maxWidth calculation
      }
      nPEs/=gain;
      peakHeight=(baseline-peakHeight)/gain;
      peakNegativeHeight=fabs((baseline-peakNegativeHeight)/gain);

      // add the last peak
      if (tEnd!=0)
         summary.AddPeak(nPEs,tThresholdUp-dT-dT_trigger,
               tStart-dT-dT_trigger,tEnd-dT-dT_trigger,
               tRise,tFall,tAtSummit-dT-dT_trigger,peakHeight,
               peakNegativeHeight,maxWidth,peakStatus);

      if (fSoftwareZLE) {
         // 0-suppress samples from the last peak to the end of wf
         for (int j=static_cast<int>(tEnd); j<nSamples; j++)
            samples[j]=baseline;

         // wf with no peak at all should not be skipped, 
         // since this may just due to some bugs in peak finding process
         //if (summary.GetNPeaks()==0) continue;

         format=1; // mark it as 0-suppressed
      }

      // add a decoded waveform
      XWaveform aWaveform(nSamples,samples,cable,pmtId,status,format);
      aWaveform.SetPedestal(baseline);
      aWaveform.Set1PEMean(gain);
      aWaveform.Set1PEMean400(gain400);
      aWaveform.SetTimeOffset(dT+dT_trigger);
      aWaveform.SetTriggerTime(triggerTime);
      wfs->AddWF(aWaveform);

      // add summary of this wf
      if (summary.GetNPeaks()>0) summaries->AddSummary(summary);

      // add raw summary of this wf
      for (short np=0; np<npeaks; np++) {
         tThresholdUp=-9999*ns; peakHeight=9999; nPEs=0; tAtSummit=0;
         for (short is=tpstart[np]; is<tpend[np]; is++) {
            iNext = ((is<nSamples-1)?(is+1):(nSamples-1));
            if (baseline-samples[is]>3 && baseline-samples[iNext]>3)
               if (tThresholdUp==-9999*ns) tThresholdUp=is*ns;
            nPEs+=rawSum.GetPedestal()-samples[is];
            if (samples[is]<peakHeight) {
               peakHeight=samples[is];
               tAtSummit=is*ns;
            }
            if (samples[is]==0) peakStatus=XWFSummary::kIsSaturated;
         }
         nPEs/=gain;
         peakHeight=(rawSum.GetPedestal()-peakHeight)/gain;
         rawSum.AddPeak(nPEs,tThresholdUp-dT-dT_trigger,
               tpstart[np]*ns-dT-dT_trigger,tpend[np]*ns-dT-dT_trigger,
               tRise,tFall,tAtSummit-dT-dT_trigger,peakHeight,peakStatus);
      }
      if (rawSum.GetNPeaks()>0) rawSums->AddSummary(rawSum);
   }

   // fill FADC header based on crate number
   header->SetCount(crate, fadcBuffer->event_count);
   header->SetTriggerTime(crate, fadcBuffer->trigger_time);
   header->SetLVDSPattern(crate, fadcBuffer->LVDS_pattern);
   header->SetChannelMask(crate, fadcBuffer->channel_mask);

   delete[] content;
}

