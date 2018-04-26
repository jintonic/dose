#ifndef PULSE_H
#define PULSE_H

#include <TObject.h>

namespace NICE { class Pulse; }

class NICE::Pulse : public TObject
{
   public:
      int bgn; ///< begin position in the waveform
      int end; ///< end position in the waveform
      int ithr; ///< position of threshold crossing point
      int ih; ///< position of max height
      double h; ///< pulse height
      double npe; ///< number of photoelectrons
      bool isSaturated;

      Pulse(): TObject(), bgn(0), end(0), ithr(0), ih(0), h(0), npe(0) {};
      Pulse(const Pulse& pls);
      Pulse& operator=(const Pulse& pls);
      virtual ~Pulse() {};

      ClassDef(Pulse,1);
};

#endif
