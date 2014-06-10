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
      enum Type {
         kCalibrated,
         kRaw,
      };
      Type type; 

      /**
       * Sampling frequency
       */
      double freq; // sampling frequency
      /**
       * Pedestal
       */
      double ped; // pedestal
      /**
       * RMS of pedestal
       */
      double prms; // RMS of pedestal
      /**
       * 32 bits trigger count
       */
      unsigned int ctrg; // 32 bits trigger count

      /**
       * Waveform samples
       */
      std::vector<double> samples; // waveform samples
      /**
       * Array of pulses
       */
      std::vector<Pulse> pulses; // array of pulses

      /**
       * Information of related PMT
       */
      PMT pmt; // information of related PMT

      WF(): type(kRaw), freq(0), ped(0), prms(0), ctrg(0) {};
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

      ClassDef(WF,1);
};

#endif
