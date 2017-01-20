#include <cmath>
using namespace std;

#include <NICE/WF.h>
using namespace NICE;

#include <UNIC/Units.h>
using namespace UNIC;

#include "Reader.h"

Reader::Reader(int run, int sub, const char *dir) :
WFs(run), Logger(), fRaw(0), fNttt(0), fNwarnings(0)
{
   fPath = Form("%s/%04d00", dir, run/100);
   fName = Form("run_%06d.%06d", run, sub);
   fData = new uint32_t[400000];
   Index();
   nch=Nch;
   thr=3;// set default threshold for software pulse scanning
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
   if (fileSize<sizeof(EVT_HDR_t)) {
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
      EVT_HDR_t hdr;
      fRaw->read(reinterpret_cast<char *> (&hdr), sizeof(EVT_HDR_t));
      if (hdr.size<=sizeof(EVT_HDR_t)) {
         fSize.push_back(hdr.size);
         Error("Index", "entry %zu is too small, stop indexing", fBegin.size());
         break;
      }
      fSize.push_back(hdr.size);

      //skip the rest of the block
      fRaw->seekg(hdr.size-sizeof(EVT_HDR_t), ios::cur);
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
      EVT_HDR_t hdr;
      fRaw->read(reinterpret_cast<char *> (&hdr), sizeof(EVT_HDR_t));

      if (hdr.type==0) {
         ReadRunCfg(i);
      } else {
         if (hdr.ttt<0) hdr.ttt=-hdr.ttt;
         if (hdr.cnt>id && int(hdr.ttt)<cnt) fNttt++;
         id = hdr.cnt;
         cnt = hdr.ttt;
         t = (fNttt*2147483647.+cnt)*8*ns;
         ReadEvent(i);
      }
   }
}

//------------------------------------------------------------------------------

void Reader::ReadRunCfg(int i)
{
   RUN_CFG_t cfg;
   fRaw->read(reinterpret_cast<char *> (&cfg), sizeof(RUN_CFG_t));

   if ((uint16_t)run!=cfg.run) {
      Warning("ReadRunCfg", "unmatched run number: %d", cfg.run);
      run=cfg.run;
   }
   id=-1; // use negative event id to indicate that it is not a real event
   sub=cfg.sub;
   t0=cfg.tsec+cfg.tus*1e-6;
   nmax=cfg.ns;
   for (int i=0; i<nch; i++) if ((cfg.mask>>i & 0x1) == 0) At(i)->pmt.id=-1;
   nfw=100; // set fw smpl not to be suppressed
   nbw=100; // set bw smpl not to be suppressed
}

//------------------------------------------------------------------------------

void Reader::ReadEvent(int i)
{
   size_t nWords=0, nTotal=fSize[i]-sizeof(EVT_HDR_t);
   fRaw->read(reinterpret_cast<char *> (fData), nTotal);
   for (unsigned short ch=0; ch<nch; ch++) {
      WF* wf = At(ch); // must have been initialized
      if (wf->pmt.id==-1) continue; // skip empty channel
      wf->Reset(); wf->freq=1*GHz; // reset waveform

      int nsmpl=0, nzip;
      while (nsmpl<nmax && nWords<nTotal/4) {
         nzip = fData[nWords]>>30 & 0x3;
         for (int i=0; i<nzip; i++)
            wf->smpl.push_back(float(fData[nWords]>>10*i & 0x3FF));
         nsmpl+=nzip;
         nWords++;
      }
   }
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
   if (wf->smpl.size()<96) return; // samples not enough

   // calculate pedestal; flip pulse; ADC -> npe
   Calibrate(ch, 96);

   // When the waveform rises from pedestal, we should either create a new
   // pulse or update the end of previous pulse, depending on whether the
   // rising point is still in the tail of previous pulse
   bool aboveThr, outOfPrevPls, prevSmplAboveThr=false;
   for (int i=0; i<nmax; i++) {
      // check if above threshold
      int next1 = (i<nmax-1)?(i+1):(nmax-1); // index of next sample
      int next2 = (i<nmax-2)?(i+2):(nmax-1); // index of next next sample
      if (  wf->smpl[i]    >thr/wf->pmt.gain &&
            wf->smpl[next1]>thr/wf->pmt.gain &&
            wf->smpl[next2]>thr/wf->pmt.gain &&
            next2>next1 && next1>i)  aboveThr=true;
      else aboveThr=false;

      // check previous pulse tentative end
      if (wf->np>0) if (i==wf->pls.back().end) if (aboveThr) 
         wf->pls.back().end=i+nfw<nmax?i+nfw:nmax-1;

      // check if out of previous pulse
      outOfPrevPls=true;
      if (wf->np>0) if (i-nbw<wf->pls.back().end) outOfPrevPls=false;

      if (aboveThr && !prevSmplAboveThr) {
         if (outOfPrevPls) { // create a new pulse
            Pulse pls;
            pls.ithr=i;
            pls.bgn=i-nbw<0?0:i-nbw;
            pls.end=i+nfw<nmax?i+nfw:nmax-1; // tentative end
            wf->pls.push_back(pls);
            wf->np++;
         } else  // update previous pulse tentative end
            wf->pls.back().end=i+nfw<nmax?i+nfw:nmax-1;
      }

      prevSmplAboveThr=aboveThr; // flip flag for next sample after using it
   }

   // update pulses
   for (int i=0; i<wf->np; i++) {
      for (int j=wf->pls.at(i).bgn; j<wf->pls.at(i).end; j++) {
         if (wf->smpl[j]>wf->pls.at(i).h) {
            wf->pls.at(i).h=wf->smpl[j];
            wf->pls.at(i).ih=j;
         }
         if (wf->smpl[j]==wf->ped/wf->pmt.gain) wf->pls.at(i).isSaturated=true;
         wf->pls.at(i).npe+=wf->smpl[j];
      }
   }
}

//------------------------------------------------------------------------------

void Reader::Calibrate(unsigned short ch, unsigned short nSamples)
{ 
   WF* wf = At(ch); // must have been filled
   if (wf->pmt.id==-1) return; // skip empty channel

   if (wf->smpl.size()<100) return; // samples not enough

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
   double integral=0;
   for (int i=0; i<nmax; i++) {
      wf->smpl[i] = (wf->ped - wf->smpl[i])/wf->pmt.gain;
      integral+=wf->smpl[i];
   }
   wf->npe=integral;
}
