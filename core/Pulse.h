#ifndef PULSE_H
#define PULSE_H

namespace NICE { class Pulse; }

class NICE::Pulse
{
   public:
      /**
       * Begin position in the waveform
       */
      int bgn; // begin position in the waveform
      /**
       * End position in the waveform
       */
      int end; // end position in the waveform

      Pulse(): bgn(0), end(0) {};
      virtual ~Pulse() {};
};

#endif
