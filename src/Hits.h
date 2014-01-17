#ifndef HITS_H
#define HITS_H

#include "Hit.h"

namespace OOPA {
   class Hits;
}

class OOPA::Hits : public TClonesArray
{
   protected:
      Float_t fTotalNPEs;
      TVector3 fCenterOfLight;

   public:
      Hits() : TClonesArray, fTotalNPEs(0), fCenterOfLight(0) {};
      virtual ~Hits() {};

      ATMHit* Add(sort_atm *atm_hit=0) { return 0; }

      TVector3 CenterOfLight() { return fCenterOfLight; }

      Float_t TotalNPEs() { return TotalNPEs; }

      /**
       * Get number of hits.
       */
      UShort_t N() { return GetEntries(); }

      ClassDef(Hits,1);
};

#endif
