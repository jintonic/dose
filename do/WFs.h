#ifndef WFS_H
#define WFS_H

#include "WF.h"

namespace NICE { class WFs; }

class NICE::WFs : public TClonesArray
{
   public:
      WFs() : TClonesArray {};
      virtual ~WFs() {};

      Int_t run, sub, evt;

      WF* Add(WF *wf=0) { return 0; }

      ClassDef(WFs,1);
};

#endif
