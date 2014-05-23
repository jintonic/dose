#ifndef WF_H
#define WF_H

#include "PMT.h"
#include "DGTZ.h"
#include <TClonesArray.h>

namespace NICE { class WF; }

class NICE::WF
{
   public:
      enum Status {
         kNoisy,
         kNormal=0,
      }

      Status st;

      std::vector<Double_t> sample;

      TClonesArray pulses;
      PMT pmt;
      DGTZ dgtz;

      WF() {};
      virtual ~WF() {};

      ClassDef(WF,1);
};

#endif
