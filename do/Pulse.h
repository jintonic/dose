#ifndef PULSE_H
#define PULSE_H

namespace NICE {
   class Pulse;
}

class NICE::Pulse
{
   public:
      enum Status {
         kNormal,
         kSaturated,
      }

      Short_t id;
      Status st;

      Int_t begin, end;

      Pulse() {};
      virtual ~Pulse() {};

      ClassDef(Pulse,1);
};

#endif
