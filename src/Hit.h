#ifndef HIT_H
#define HIT_H

#include "ATMHit.h"

namespace OOPA {
   class Hit; 
   class Hits; 
}

class OOPA::Hit : public ATMHit
{
   protected:
      Float_t fNPEs;
      Float_t fTime;

   public:
      Hit() : ATMHit(), fNPEs(0), fTime(0) {};
      virtual ~Hit() {};

      /**
       * Reconstruct a physics hit from it digital information.
       */
      void Reconstruct() {}

      ClassDef(Hit,1);

      friend class Hits;
};

#endif
