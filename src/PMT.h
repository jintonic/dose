#ifndef PMT_H
#define PMT_H

#include "ATM.h"
#include "FADC.h"

namespace OOPA {
   class PMT;
   enum PMTStatus {
      kNotExist,
      kFlashing,
      kDead,
      kNormal=0,
      kLowGain,
   }
}

class OOPA::PMT : public ATM, FADC
{
   protected:
      UShort_t fPMTId;
      TVector3 fXYZ;
      TVector3 fNormVector;
      PMTStatus fStatus;

   public:
      PMT() : ATM(), FADC() {};
      virtual ~PMT() {};

      ClassDef(PMT,1);
};

#endif
