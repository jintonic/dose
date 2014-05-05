#ifndef WF_H
#define WF_H

#include "PMT.h"
#include <TClonesArray.h>

namespace NICE { class WF; }

class NICE::WF
{
   public:
      enum Status {
         kNoisy,
         kNormal=0,
      }

      Short_t id, ch, mo, cr;
      Status st;

      std::vector<Double_t> sample;

      TClonesArray pulses;
      PMT pmt;

      WF() {};
      virtual ~WF() {};

      ClassDef(WF,1);
};

#endif
