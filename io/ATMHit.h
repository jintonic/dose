#ifndef ATMHIT_H
#define ATMHIT_H

#include "PMT.h"

namespace OOPA {
   class ATMHit; 
   class ATMHits; 
}

class OOPA::ATMHit : public PMT
{
   protected:
      Float_t fADC;
      Float_t fTDC;

   public:
      ATMHit() : PMT(), fADC(0), fTDC(0) {};
      virtual ~ATMHit() {};

      ClassDef(ATMHit,1);

      friend class ATMHits;
};

#endif
