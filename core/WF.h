#ifndef WF_H
#define WF_H

#include <vector>

#include "PMT.h"
#include "Pulse.h"

namespace NICE { class WF; }

class NICE::WF : public PMT
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
       * Trigger time
       */
      double ttrg; // trigger time

      /**
       * Waveform samples
       */
      std::vector<double> samples; // waveform samples
      /**
       * Array of pulses
       */
      std::vector<Pulse> pulses; // array of pulses

      WF(): PMT(), type(kRaw), freq(0), ped(0), prms(0), ttrg(0) {};
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
