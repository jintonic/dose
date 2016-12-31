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
      int run; ///< run number
      short sub; ///< sub run number
      int id; ///< event id (number of triggered events)
      int cnt; ///< trigger count
      double t; ///< nano sec calculated from trigger count
      /**
       * Seconds to Unix epoch
       * Valid till year 2038. = sec + usec*1e-6 
       */
      double sec;
      /**
       * Total number of available channels
       * Filled after loading PMT database
       */
      unsigned short nch;
      /**
       * Maximal number of samples of a waveform
       * Filled after loading run header
       */
      unsigned short nmax;
      /**
       * Number of samples after non-0-suppressed window
       * Filled after loading run header
       */
      unsigned short nfw;
      /**
       * Number of samples before non-0-suppressed window
       * Filled after loading run header
       */
      unsigned short nbw;
      /**
       * Threshold for software 0-suppression
       * unit: ADC count
       */
      unsigned short thr;
      TClonesArray wf; ///< waveform array

   public:
      WFs(int run=999999) : TObject(), run(run), sub(-1), id(-1), cnt(-1),
      t(-1), sec(-1), nch(8), nmax(0), nfw(0), nbw(0), thr(0) {};
      virtual ~WFs() {};

      void Initialize(const char *db="/path/to/pmt");

      WF* At(unsigned short i) const;
      WF* operator[](unsigned short i) const { return At(i); }

      ClassDef(WFs,2);
};

#endif
