#ifndef WF_H
#define WF_H

#include <vector>

#include "PMT.h"
#include "Pulse.h"

namespace NICE { class WF; }
/**
 * Waveform information.
 *
 * It uses PMT instead of inheritting it so that tree->Show(1) lists items like
 * wfs.pmt.id instead of wfs.id.
 *
 * It contains a vector of Pulse instead of a TClonesArray for simplicity.
 * Nested TClonesArray cannot be split and loose all benifit of TClonesArray:
 * http://root.cern.ch/phpBB3/viewtopic.php?f=3&t=910
 */
class NICE::WF : public TObject
{
   public:
      double freq; ///< sampling frequency
      double ped;  ///< pedestal
      double prms; ///< RMS of pedestal
      unsigned int ctrg; ///< 32 bits trigger count
      std::vector<double> smpl; ///< waveform samples
      unsigned short ns; ///< number of samples
      std::vector<Pulse> pls; ///< array of pulses
      unsigned short np; ///< number of pulses
      double npe; ///< total number of photoelectrons
      PMT pmt; ///< information of related PMT

      WF(): TObject(),freq(0),ped(0),prms(0),ctrg(0),ns(0),np(0),npe(0){};
      virtual ~WF() {};

      bool IsSimilarTo(const WF& other) const;
      void MakeSimilarTo(const WF& other);

      WF& operator+=(const WF& other); 
      WF& operator-=(const WF& other); 
      WF& operator*=(const WF& other); 
      WF& operator/=(const WF& other); 

      WF& operator+=(double value); 
      WF& operator-=(double value) { return (*this) += -value; }
      WF& operator*=(double value); 
      WF& operator/=(double value);

      double GetIntegral(short start, short end); ///< Integrate in [start, end)

      Pulse* GetPulse(unsigned short idx=0); ///< Get the idx pulse

      void Reset();

      ClassDef(WF,1);
};

#endif
