#ifndef WFS_H
#define WFS_H

#include <TClonesArray.h>

#include "WF.h"

namespace NICE { class WFs; }

class NICE::WFs : public TClonesArray
{
   public:
      int run, sub, evt;
      double time;

   public:
      WFs(int run=-1) : TClonesArray(), run(run) {};
      virtual ~WFs() {};

      void LoadDatabase(const char *db="/path/to/db");

      WF* Add(WF *wf=0) { return 0; }

      ClassDef(WFs,1);
};

#endif
