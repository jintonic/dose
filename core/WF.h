#ifndef WF_H
#define WF_H

#include <vector>

#include <TClonesArray.h>

#include "PMT.h"
#include "Pulse.h"

namespace NICE { class WF; }
/**
 * Waveform information.
 * It uses PMT instead of inheritting it so that tree->Show(1) lists items like
 * wfs.pmt.id instead of wfs.id.
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
      TClonesArray pulses; // array of pulses

      /**
       * Information of related PMT
       */
      PMT pmt; // information of related PMT

      WF(): type(kRaw), freq(0), ped(0), prms(0), ttrg(0) {};
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
