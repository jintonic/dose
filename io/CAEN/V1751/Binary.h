#ifndef CAEN_V1751_H
#define CAEN_V1751_H

#include <fstream>
#include <vector>

#include <WFs.h>
namespace NICE { class CAEN_V1751; }

class NICE::CAEN_V1751: public NICE::WFs, NICE::Logger
{
   private:
      ifstream *fRaw;
      TString fPath, fName;
      std::vector<ifstream::pos_type> fBegin;
      std::vector<streamoff> fSize;

      typedef struct eventhead {
         UInt_t size;
         UInt_t run_no;
         UInt_t event_no;
      } fEvtHdr;

   public:
      Int_t entries;

      CAEN_V1751(Int_t run, Int_t sub, const char *dir);
      virtual ~CAEN_V1751() {};

      void Close() { if (fRaw) if (fRaw->is_open()) fRaw->close(); }

      void GetEntry(UInt_t i);

      void Index();
      void DumpIndex();
      void LoadIndex();

      ClassDef(CAEN_V1751,0);
};
#endif
