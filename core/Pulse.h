#ifndef PULSE_H
#define PULSE_H

#include <TObject.h>

namespace NICE { class Pulse; }

class NICE::Pulse : public TObject
{
   public:
      enum {
         kSaturated = BIT(14),
      };

      /**
       * Begin position in the waveform
       */
      int bgn; // begin position in the waveform
      /**
       * End position in the waveform
       */
      int end; // end position in the waveform
      /**
       * Position of threshold crossing point
       */
      int ithr; // position of threshold crossing point
      /**
       * Position of max height
       */
      int ih; // position of max height
      /**
       * Pulse height
       */
      double h; // pulse height
      /**
       * Number of photoelectrons
       */
      double npe; // number of photoelectrons

      Pulse();
      Pulse(const Pulse& pls);
      Pulse& operator=(const Pulse& pls);
      virtual ~Pulse() {};

      ClassDef(Pulse,1);
};

#endif
