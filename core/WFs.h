#ifndef WFS_H
#define WFS_H

#include <TClonesArray.h>

namespace NICE {
   class WF;
   class WFs;
}
/**
 * Information of waveform array
 * It contains some global information common for all channels.
 *
 * It uses TClonesArray instead of inheriting from it because TClonesArray is
 * created merely as a container.
 *
 * It loads PMT database to initialize the TClonesArray of empty waveforms with
 * PMT information. Loading database is done here instead of in the NICE::PMT
 * class to avoid passing run, file name and location to NICE::PMT.
 */
class NICE::WFs : public TObject
{
   private:
      WF* Map(int ch); ///< map ch to WF::pmt
      void LoadTimeOffset(WF* wf); ///< load dt offset to WF::pmt
      void LoadMeanOf1PE(WF* wf); ///< load 1PE mean to a WF::pmt
      void LoadStatus(WF* wf); ///< load status to a WF::pmt

   protected:
      /**
       * Path to PMT database
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
      unsigned int cnt; // trigger count
      /**
       * Seconds to Unix epoch
       * Valid till year 2038
       */
      unsigned int sec; // Linux time of event in second
      /**
       * Total number of available channels
       * Filled after loading PMT database
       */
      unsigned short nch; // number of available channels
      /**
       * Maximal number of samples of a waveform
       * Filled after loading run header
       */
      unsigned short nmax; // max number of samples of a wf
      /**
       * Number of samples after non-0-suppressed window
       * Filled after loading run header
       */
      unsigned short nfw; // # of samples after non-0-suppressed window
      /**
       * Number of samples before non-0-suppressed window
       * Filled after loading run header
       */
      unsigned short nbw; // # of samples before non-0-suppressed window
      /**
       * Threshold for software 0-suppression
       * unit: ADC count
       */
      unsigned short thr; // threshold for software 0-suppression
      /**
       * Waveform array
       */
      TClonesArray wfs; // waveform array

   public:
      WFs(int run=999999);
      virtual ~WFs() {};

      void Initialize(const char *db="/path/to/pmt");

      ClassDef(WFs,1);
};

#endif
