#ifndef WFS_H
#define WFS_H

#include <TClonesArray.h>

#include "WF.h"

namespace NICE { class WFs; }

class NICE::WFs : public TClonesArray
{
   public:
      /**
       * Run number
       */
      int run; // run number
      /**
       * Sub run number
       */
      int sub; // sub run number
      /**
       * Event id
       */
      int evt; // event id
      /**
       * Linux time of event
       */
      double time; // Linux time of event

   public:
      WFs(int run=999999) : TClonesArray(), run(run) {};
      virtual ~WFs() {};

      void LoadDatabase(const char *db="/path/to/db");

      WF* Add(WF *wf=0) { return 0; }

      ClassDef(WFs,1);
};

#endif
