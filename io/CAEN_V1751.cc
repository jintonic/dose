using namespace std;

#include <Units.h>
using namespace UNIC;

#include "CAEN_V1751"

CAEN_V1751::CAEN_V1751(Int_t run=0, Int_t sub=0, const char *dir=".") :
WFs(), Logger(), fRaw(0), entries(0)
{
   fPath = Form("%s/%04d00/%06d", dir, run/100, run);
   fName = Form("run_%06d.%06d", run, sub);
   Index();
   LoadDatabase();
}

//------------------------------------------------------------------------------

void CAEN_V1751::Index()
{
   LoadIndex();
   if (entries>0) return;

   // open data file in a read-only mode and put the "get" pointer to the end
   // of the file
   fRaw = new ifstream(fName.Data(), ios::in | ios::binary | ios::ate); 
   if (!fRaw->is_open()) { //stream is not properly connected to the file
      Warning("Index", "%s/%s cannot be read!", fPath.Data(), fName.Data()); 
      return; 
   } else {
      Info("Index", "%s/%s", fPath.Data(), fName.Data()); 
   }

   // get the position of the "get" pointer, since the "get" pointer is at the
   // end of the file, its position equals the size of the data file
   ifstream::pos_type maximum = fRaw->tellg();
   Info("Index","file size: %d Byte", Int_t(maximum));

   // check file size
   if (maximum<sizeof(EVENTHEAD)) {
      Warning("Index", "binary file size is too small!"); 
      fRaw->close();
      return;
   }

   // move the "get" pointer back to the beginning of the file, from where
   // reading starts
   fRaw->seekg(0, ios::beg);

   // get total number of events and save the starting points of events into a
   // vector for later use
   Int_t blockSize=0;
   fBegin.resize(0);
   fSize.resize(0);
   while (fRaw->good() && fRaw->tellg()<maximum) {
      //read event header
      EVENTHEAD header;
      fRaw->read(reinterpret_cast<char *> (&header), sizeof(EVENTHEAD));

      //save starting points
      fBegin.push_back(fRaw->tellg());

      //skip the rest of the block
      blockSize=header.size-sizeof(EVENTHEAD);
      fRaw->seekg(blockSize, ios::cur);
      fSize.push_back(blockSize);
   }
   entries=fBegin.size();
}

//------------------------------------------------------------------------------

void DumpIndex()
{
   for (UInt_t i=0; i<fBegin.size(); i++)
      Printf("%20d %d", fBegin[i], fSize[i]);
}

//------------------------------------------------------------------------------

void CAEN_V1751::GetEntry(UInt_t i)
{
   fRaw->seekg(fBegin[i], ios::beg);

   while (fRaw->tellg()<fBegin[i]+fSize[i]) {
      // read buffer header
      BUFHEAD head;
      fRaw->read(reinterpret_cast<char *> (&head), sizeof(FADC_BUFHEAD));

      // decode buffer header
      run = head.run_number;
      evt = head.serial_event_number;
      time = head.time.tv_sec*second + head.time.tv_usec*us;
      UInt_t  dataSize  = head.data_size;
      UInt_t  bufferType= head.flag;
      UInt_t fadcFlag = head.fadc_flag;
      UInt_t fadcError= head.fadc_error_flag;

      if (bufferType==RUN_INFO) {
         // move getter back to the beginning of this buffer
         fRaw->seekg(-(Int_t)sizeof(FADC_BUFHEAD),ios::cur);
         // read the whole buffer
         char *buffer = new char[dataSize];
         fRaw->read(buffer, dataSize);

         DecodeRUNINFOBUF(buffer);

         delete[] buffer;
      } else if (bufferType==FADC_INFO) {
         // move getter back to the beginning of this buffer
         fRaw->seekg(-(Int_t)sizeof(FADC_BUFHEAD),ios::cur);
         // read the whole buffer
         char *buffer = new char[dataSize];
         fRaw->read(buffer, dataSize);

         DecodeFADCINFOBUF(buffer);

         delete[] buffer;
      } else if (bufferType==NORMAL_FADC_FLAG) {
         // move getter back to the beginning of this buffer
         fRaw->seekg(-(Int_t)sizeof(FADC_BUFHEAD),ios::cur);
         // read the whole buffer
         char *buffer = new char[dataSize];
         fRaw->read(buffer, dataSize);

         DecodeFADCBUF(buffer);

         delete[] buffer;
      } else {
         // skip the rest of the buffer
         fRaw->seekg(dataSize-(Int_t)sizeof(FADC_BUFHEAD),ios::cur);
      }
   }
}

//------------------------------------------------------------------------------

void CAEN_V1751::DecodeFADCINFOBUF(const char* buffer)
{
   FADCINFOBUF* infobuf = (FADCINFOBUF *) buffer;

   FADCINFO info = infobuf->fadcinfo;

   XFADCHeader *header = fEvent->GetFADCHeader();
   header->SetTotalTimeWindow(info.custom_size*8*ns);
   header->SetForwardTimeWindow(info.nsamples_lfw*8*ns);
   header->SetBackwardTimeWindow(info.nsamples_lbk*8*ns);
   header->SetUpperThreshold(info.threshold_upper);
   header->SetLowerThreshold(info.threshold_under);
}

//------------------------------------------------------------------------------

void CAEN_V1751::DecodeRUNINFOBUF(const char* buffer)
{
   RUNINFOBUF* infobuf = (RUNINFOBUF *) buffer;

   RUNINFO info = infobuf->runinfo;

   // decode time info
   struct tm* ptm = 0;
   union {time_t in_time_t; int in_int;} thisTime = {0}; // convert int to time_t
   char startTime[80], currentTime[80];

   thisTime.in_int = info.start_time;
   ptm = localtime(&(thisTime.in_time_t));
   strftime(startTime, sizeof(startTime), "%Y-%m-%d %H:%M:%S", ptm);

   thisTime.in_int = info.current_time;
   ptm = localtime(&(thisTime.in_time_t));
   strftime(currentTime, sizeof(currentTime), "%Y-%m-%d %H:%M:%S", ptm);

   // print decoded run info
   if (GetVerbosity()>4) {
      cout<<"  runinfo.run_mode:     "<<info.run_mode<<endl;
      cout<<"  runinfo.run_title:    "<<info.run_title<<endl;
      cout<<"  runinfo.shift_leader: "<<info.shift_leader<<endl;
      cout<<"  runinfo.shift_member: "<<info.shift_member<<endl;
      cout<<"  runinfo.start_time:   "<<startTime<<endl;
      cout<<"  runinfo.current_time: "<<currentTime<<endl;
      cout<<"  runinfo.daq_version:  "<<info.daq_version<<endl;
   }

   XFADCHeader *header = fEvent->GetFADCHeader();
   header->SetMode(info.run_mode);
   header->SetVersion(info.daq_version);
}

//______________________________________________________________________________
//

void CAEN_V1751::DecodeFADCBUF(const char* buffer)
{
   // prepare FADC header
   XFADCHeader *header = fEvent->GetFADCHeader();
   // prepare wf arrays for filling
   XWaveforms *wfs = fEvent->GetIDData()->GetWFs();
   // prepare wf summary arrays for raw wfs
   XWFSummaries *rawSums = fEvent->GetIDData()->GetRawWFSummaries();
   // prepare wf summary arrays for 0-suppressed wfs
   XWFSummaries *summaries = fEvent->GetIDData()->GetWFSummaries();

   // get total number of raw waveforms
   FADCBUF *fadcBuffer = (FADCBUF *) buffer;
   UInt_t totalNWFs = fadcBuffer->entry;
   if (totalNWFs>=FADC_NBOARDS*FADC_NCHANNELS) {
      Warning("DecodeFADCBUF","number of waveforms = %d, return!",totalNWFs);
      fEvent->GetHeader()->SetStatus(XEventHeader::kFADCProblem);
      return;
   }
   if (GetVerbosity()>5) cout<<"fadcbuf.entry:       "<<totalNWFs<<endl;

   // move pointer to the beginning of waveform data block
   char* ptr = (char*) &(fadcBuffer->fadc[0].daq_id);

   Double_t dT_trigger = header->GetTriggerJitter();
   if (dT_trigger==-999) {
     // loop through each raw waveform
     UShort_t cable, crate=0, module, channel;
     Int_t ntrigger_channels = 0;
     for (UInt_t iwf=0; iwf<totalNWFs; iwf++) {
       // decode cable number
       cable   = *((UShort_t*)ptr);
       crate   = cable>>10 & 0x0000000F;
       module  = cable>>5  & 0x0000001F;
       channel = cable     & 0x0000000F;
       cable   = crate*FADC_NBOARDS*FADC_NCHANNELS+module*FADC_NCHANNELS+channel;

       // get data format, it is always 0
       Short_t format = *((Short_t*) (ptr+=2));
	 
       // get data size in unit of byte
       UInt_t size = *((UInt_t*) (ptr+=2));
	 
       // decode waveform data
       UInt_t *data = (UInt_t*) (ptr+=4);
       ptr+=size; // move pointer to next raw waveform

       if (crate==0 && module==14) {
          // if no PMT connected to this cable
          Short_t pmtId=-1; 
          XWaveform::EPMTStatus status=XWaveform::kInEmptyChannel;

          UInt_t dataSize = data[0]; // data size in unit of integer

          // baselineFlag==0 -> integer baseline, ==1 -> float baseline
          Int_t baselineFlag = (data[2]>>31)&0x1;
          // 2^(baselineNsam+2)= number of samples used for baseline calculation
          Int_t baselineNsam = (data[2]>>28)&0x7; 
          Int_t baselineSum = data[2]&0x7ffff; // sum of ADC counts
          Double_t baseline; // baseline for this channel
          if (baselineFlag==0) baseline = data[2]; // integer
          else baseline = baselineSum/pow(2.,baselineNsam+2);

          UInt_t nSamples=0; // total number of samples
          UInt_t nSuppressed=0; // number of suppressed samples
          Double_t samples[12000]={0}; // array to save samples
          for (UInt_t i=3;i<dataSize;i++) {
             UInt_t nZippedSamples = (data[i]>>30);
             if (nZippedSamples==3) { // got 3 zipped samples
                // decode 3 samples
                samples[nSamples]   = (data[i])     & 0x000003FF;
                samples[nSamples+1] = (data[i]>>10) & 0x000003FF;
                samples[nSamples+2] = (data[i]>>20) & 0x000003FF;
                // increase number of samples
                nSamples += 3;
             } else if (nZippedSamples==0) { // no zipped samples
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
                UInt_t nSkippedSamples = data[i]*8;
                if (nSkippedSamples+nSamples>=12000) {
                   Warning("DecodeFADCBUF",
                         "number of skipped samples >=12000, break!");
                   fEvent->GetHeader()->SetStatus(XEventHeader::kFADCProblem);
                   break;
                }
                // fill skipped samples with baseline
                nSuppressed+=nSkippedSamples;
                for (UInt_t j=0; j<nSkippedSamples; j++) {
                   samples[nSamples]=baseline;
                   nSamples++;
                }
             } else { // skip the rest samples if nZippedSamples!=0 or 3
                if (fEvent->GetFADCHeader()->GetRun()>6959)
                   Warning("DecodeFADCBUF",
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
          Double_t totalWindow=fEvent->GetFADCHeader()->GetTotalTimeWindow();
          if (pmtId>0 && nSamples*ns!=totalWindow && GetVerbosity()>6) {
             Warning("DecodeFADCBUF",
                   "total number of samples is %4.0f", totalWindow);
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

          XWaveform aWaveform(nSamples,samples,cable,pmtId,status,format);
          wfs->AddWF(aWaveform);
          ntrigger_channels++;
       }
     }

     if (wfs->GetTriggerOutWF()->GetNSamples()>0 &&
           wfs->GetFADCTriggerWF()->GetNSamples()>0 ) {
        Double_t fTimeTRG  = wfs->GetTriggerOutWF()->GetCrossingTLocal(fLogicThreshold);
        Double_t fTimeFADC = wfs->GetFADCTriggerWF()->GetCrossingTLocal(fLogicThreshold);
        dT_trigger =  fTriggerTimeDelay + fTimeTRG - fTimeFADC;
        header->SetTriggerJitter(dT_trigger);
     }
     if(wfs->GetIDDiscriminatorWF()->GetNSamples()>0){
        header->SetIDDiscriminatorOut( wfs->GetIDDiscriminatorWF()->GetCrossingTLocal(fLogicThreshold));
     }
     if(wfs->GetODDiscriminatorWF()->GetNSamples()>0){
        header->SetODDiscriminatorOut( wfs->GetODDiscriminatorWF()->GetCrossingTLocal(fLogicThreshold));
     }
     if(wfs->GetTriggerOutWF()->GetNSamples()>0){
        header->SetTriggerOut( wfs->GetTriggerOutWF()->GetCrossingTLocal(fLogicThreshold));
     }
     if(wfs->GetFADCTriggerWF()->GetNSamples()>0){
        header->SetFADCTrigger( wfs->GetFADCTriggerWF()->GetCrossingTLocal(fLogicThreshold));
     }
   }

   // move pointer to the beginning of waveform data block
   ptr = (char*) &(fadcBuffer->fadc[0].daq_id);

   // loop through each raw waveform
   XWFSummary summary; // prepare wf summary for software 0-suppression
   XWFSummary rawSum; // prepare raw wf summary
   UShort_t npeaks=0, tpstart[200], tpend[200];

   Short_t cable, crate=0, module, channel;
   for (UInt_t iwf=0; iwf<totalNWFs; iwf++) {
      // decode cable number
      cable   = *((Short_t*)ptr);
      crate   = cable>>10 & 0x0000000F;
      module  = cable>>5  & 0x0000001F;
      channel = cable     & 0x0000000F;
      cable   = crate*FADC_NBOARDS*FADC_NCHANNELS+module*FADC_NCHANNELS+channel;

      // if no PMT connected to this cable
      Short_t pmtId=-1; 
      Double_t gain= 1;
      Double_t gain400= 1;
      Double_t dT  = 0;
      Double_t pescale = 1;

      // if there is PMT connected to this cable
      XInnerPMT *aPMT = fDetector->GetInnerPMTs()->GetPMTByFADCCable(cable);
      if (aPMT) {
         pmtId = aPMT->GetId();
         gain = aPMT->GetFADC1PEMean();
         gain400 = aPMT->GetFADC1PEMean400();
         dT = aPMT->GetFADCdT();
         pescale = aPMT->GetPEScaleForScintillation();

         // apply PE scale factor except for LED runs
         UInt_t TriggerId = (UInt_t)fEvent->GetHeader()->GetTriggerId();
         if ( ((TriggerId>>2)&0x1)==0 && ((TriggerId>>5)&0x1)==0 ) {
            if (GetVerbosity()>5)
               cout << "TriggerId= " << TriggerId << ", PE scale = " << pescale << endl;
            gain /= pescale;
            gain400 /= pescale;
         }
      }

      // get PMT status
      XWaveform::EPMTStatus status;
      if (pmtId<0) 
         status=XWaveform::kInEmptyChannel;
      else if (aPMT->GetStatus()==XPMT::kOK) 
         status=XWaveform::kInGoodPMT;
      else if (aPMT->GetStatus()==XPMT::kDead) 
         status=XWaveform::kInDeadPMT;
      else if (aPMT->GetStatus()==XPMT::kNoisy) 
         status=XWaveform::kInNoisyPMT;
      else if (aPMT->GetStatus()==XPMT::kLowGain) 
         status=XWaveform::kInLowGainPMT;
      else if (aPMT->GetStatus()==XPMT::kElecProblem) 
         status=XWaveform::kInBadChannel;
      else if (aPMT->GetStatus()==XPMT::kATMProblem) 
         status=XWaveform::kHasATMProblem; 
      else if (aPMT->GetStatus()==XPMT::kFADCProblem) 
         status=XWaveform::kHasFADCProblem;
      else if (aPMT->GetStatus()==XPMT::kFlasher) 
         status=XWaveform::kInFlasher;
      else if (aPMT->GetStatus()==XPMT::kDischarge) 
         status=XWaveform::kInDischarger;
      else 
         status=XWaveform::kUnknow;

      // get data format, it is always 0
      Short_t format = *((Short_t*) (ptr+=2));

      // get data size in unit of byte
      UInt_t size = *((UInt_t*) (ptr+=2));
      if (size>1.8*1024*1024) {
         Warning("DecodeFADCBUF",
               "size of WF exceeds 1.8 MB. Break!");
         fEvent->GetHeader()->SetStatus(XEventHeader::kFADCProblem);
         break;
      }
      
      // reset peak start and end time array
      for (UShort_t np=0; np<200; np++) {
         tpstart[np]=0;
         tpend[np]=0;
      }

      // decode waveform data
      UInt_t *data = (UInt_t*) (ptr+=4);
      ptr+=size; // move pointer to next raw waveform
      UInt_t dataSize = data[0]; // data size in unit of integer
      Double_t triggerTime = data[1]*8*ns; // trigger time for this channel

      // baselineFlag==0 -> integer baseline, ==1 -> float baseline
      Int_t baselineFlag = (data[2]>>31)&0x1;
      // 2^(baselineNsam+2)= number of samples used for baseline calculation
      Int_t baselineNsam = (data[2]>>28)&0x7; 
      Int_t baselineSum = data[2]&0x7ffff; // sum of ADC counts
      Double_t baseline; // baseline for this channel
      if (baselineFlag==0) baseline = data[2]; // integer
      else baseline = baselineSum/pow(2.,baselineNsam+2);

      npeaks=0;
      Bool_t isBaseline=kTRUE;
      UInt_t nSamples=0; // total number of samples
      UInt_t nSuppressed=0; // number of suppressed samples
      Double_t samples[12000]={0}; // array to save samples
      for (UInt_t i=3;i<dataSize;i++) {
         UInt_t nZippedSamples = (data[i]>>30);
         if (nZippedSamples==3) { // got 3 zipped samples
            // decode 3 samples
            samples[nSamples]   = (data[i])     & 0x000003FF;
            samples[nSamples+1] = (data[i]>>10) & 0x000003FF;
            samples[nSamples+2] = (data[i]>>20) & 0x000003FF;

            // record peak start time
            if (isBaseline==kTRUE) {
               if (npeaks>=200) {
                  Warning("DecodeFADCBUF", "%d peaks detected,",npeaks);
                  Warning("DecodeFADCBUF", "only 200 peaks can be handled,");
                  Warning("DecodeFADCBUF", "the rest peaks won't be recorded!");
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
            UInt_t nSkippedSamples = data[i]*8;
            if (nSkippedSamples+nSamples>=12000) {
               Warning("DecodeFADCBUF",
                     "number of skipped samples >=12000, break!");
               fEvent->GetHeader()->SetStatus(XEventHeader::kFADCProblem);
               break;
            }
            // fill skipped samples with baseline
            nSuppressed+=nSkippedSamples;
            for (UInt_t j=0; j<nSkippedSamples; j++) {
               samples[nSamples]=baseline;
               nSamples++;
            }
         } else { // skip the rest samples if nZippedSamples!=0 or 3
            fNZippedWarning++;
            if (fNZippedWarning==100) Warning("DecodeFADCBUF",
                  "zipped sample warnings exceed 100 lines!");
            else if (fNZippedWarning<100) Warning("DecodeFADCBUF",
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
      Double_t totalWindow=fEvent->GetFADCHeader()->GetTotalTimeWindow();
      if (pmtId>0 && nSamples*ns!=totalWindow && GetVerbosity()>6) {
         Warning("DecodeFADCBUF",
               "number of samples in PMT %3d (cr.%d-mo.%d-ch.%d) is %d,",
               pmtId, crate, module, channel, nSamples);
         Warning("DecodeFADCBUF",
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
         Double_t realBaseline=0.; Double_t realBaselineSquare=0.;
         Double_t baselineRMS=0;  
         UShort_t nBaselineSamples_max=96; UShort_t nBaselineSamples = 0;
         if(nSamples<nBaselineSamples_max) nBaselineSamples_max = nSamples;
         for (UInt_t i=tpstart[0]; i<(tpstart[0]+nBaselineSamples_max); i++) {
           realBaseline += (double)(samples[i]);
           realBaselineSquare += (double)(samples[i]*samples[i]);
           nBaselineSamples++;
         }
         realBaseline = realBaseline/(double)(nBaselineSamples);
         realBaselineSquare = realBaselineSquare/(double)(nBaselineSamples);
         baselineRMS = sqrt(realBaselineSquare - realBaseline*realBaseline);

         Double_t realBaseline_tmp = realBaseline;
         nBaselineSamples = 0;
         realBaseline = 0.; realBaselineSquare = 0.; baselineRMS = 0.;
         int baseline_calc_flag = 0;
         Double_t sample_prev = 0; Double_t sampleSquare_prev = 0;
         for (UInt_t i=tpstart[0]; i<(tpstart[0]+nBaselineSamples_max); i++) {
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
      Double_t nPEs=0, peakHeight=9999, peakNegativeHeight=-9999,tAtSummit=0;
      Double_t maxWidth=0, tmpWidth=0;
      Int_t maxWidth_flag = 0; 
      Double_t tRise=0, tFall=0;
      Double_t tStart=0, tThresholdUp=0, tThresholdDn=0, tEnd=0;
      fEvent->GetFADCHeader()->SetBackwardTimeWindow(32*ns);
      Double_t forwardWindow=fEvent->GetFADCHeader()->GetForwardTimeWindow();
      Double_t backwardWindow=fEvent->GetFADCHeader()->GetBackwardTimeWindow();
      XWFSummary::EWFStatus peakStatus=XWFSummary::kIsOK;

      UInt_t iNext, iNextNext;
      for (UInt_t i=0; i<nSamples; i++) {
         iNext = ((i<nSamples-1)?(i+1):(nSamples-1));
         iNextNext = ((i<nSamples-2)?(i+2):(nSamples-1));
         if ( baseline-samples[i]>3 &&
               baseline-samples[iNext]>3 &&
               baseline-samples[iNextNext]>3 ) { // above threshold
            if (isBaseline==kTRUE) { // previous sample is below threshold
               // if not overlapped with previous wf
               if (i*ns-backwardWindow>tEnd) { 
                  // calculate nPEs under previous peak
                  for (UInt_t j=static_cast<UInt_t>(tStart); // previous tStart
                        j<static_cast<UInt_t>(tEnd); // previous tEnd
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
                     for (UInt_t j=static_cast<UInt_t>(tEnd); // previous tEnd
                           j<static_cast<UInt_t>(tStart); // new tStart
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
      for (UInt_t j=static_cast<UInt_t>(tStart);
            j<static_cast<UInt_t>(tEnd);
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
         for (UInt_t j=static_cast<UInt_t>(tEnd); j<nSamples; j++)
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
      for (UShort_t np=0; np<npeaks; np++) {
         tThresholdUp=-9999*ns; peakHeight=9999; nPEs=0; tAtSummit=0;
         for (UShort_t is=tpstart[np]; is<tpend[np]; is++) {
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
}

