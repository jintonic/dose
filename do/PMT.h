#ifndef PMT_H
#define PMT_H

namespace NICE { class PMT; }

class NICE::PMT
{
   public:
      enum Status {
         kNotExist,
         kFlashing,
         kDead,
         kNormal=0,
         kLowGain,
      }

      Short_t id;
      TVector3 pos;
      Status st;

      PMT() {};
      virtual ~PMT() {};

      ClassDef(PMT,1);
};

#endif
