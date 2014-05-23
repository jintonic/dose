#ifndef CAEN_V1751_H
#define CAEN_V1751_H

#include <do/WFs.h>
namespace NICE { class CAEN_V1751; }

class NICE::CAEN_V1751: public NICE::WFs
{
   public:
      CAEN_V1751(Int_t run=0, Int_t sub=0, const char *dir="."): WFs() {};
      virtual ~CAEN_V1751() {};

      TString dir, file;
      void Open(Int_t run=0, Int_t sub=0, const char *dir=".") {};
      Int_t GetEntry(Int_t entry){};
      void Close() {};
      void Index() {};
      void DumpIndex() {};

      ClassDef(CAEN_V1751,1);
};

#endif
