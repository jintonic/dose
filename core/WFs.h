#ifndef WFS_H
#define WFS_H

#include <TClonesArray.h>

#include "WF.h"

namespace NICE { class WFs; }

class NICE::WFs : public TClonesArray
{
   private:
      PMT* Map(int ch); ///< map ch to pmt
      void LoadTimeOffset(PMT* aPMT);
      void LoadMeanOf1PE(PMT* aPMT);
      void LoadStatus(PMT* aPMT);

   protected:
      /**
       * /path/to/pmt
       */
      TString fDB; //! /path/to/pmt/

   public:
      /**
       * Run number
       */
      int run; // run number
      /**
       * Sub run number
       */
      short sub; // sub run number
      /**
       * Event id
       */
      int evt; // event id
      /**
       * Trigger count
       */
      int cnt; // trigger count
      /**
       * Seconds to Unix epoch
       */
      int sec; // Linux time of event in second
      /**
       * Total number of available channels
       */
      short nch; // number of available channels
      /**
       * Maximal number of samples of a waveform
       */
      size_t nmax; // max number of samples of a wf
      /**
       * Number of samples after non-0-suppressed window
       */
      int nfw; // # of samples after non-0-suppressed window
      /**
       * Number of samples before non-0-suppressed window
       */
      int nbw; // # of samples before non-0-suppressed window
      /**
       * Threshold for software 0-suppression
       */
      int thr; // threshold for software 0-suppression

   public:
      WFs(int run=999999) :
         TClonesArray("NICE::WF", 8),
         run(run), sub(-1), evt(-1), cnt(-1), sec(-1), nmax(0), 
         nfw(0), nbw(0), thr(0) { nch=fSize; }

      virtual ~WFs() {};

      void Initialize(const char *db="/path/to/pmt");

      ClassDef(WFs,1);
};

#endif
