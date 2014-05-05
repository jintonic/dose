#ifndef HIT_H
#define HIT_H

#include "WF.h"
#include "PMT.h"

namespace NICE { class Hit; }

class NICE::Hit
{
   public:
      Float_t nPEs;
      Float_t time;

      Hit() {};
      virtual ~Hit() {};

      /**
       * Reconstruct a physics hit from it digital information.
       */
      void Reconstruct() {}

      ClassDef(Hit,1);
};

#endif
