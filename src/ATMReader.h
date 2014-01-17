#ifndef ATMREADER_H
#define ATMREADER_H

namespace OOPA {
   class ATMReader;
}

class OOPA::ATMReader
{
   private:
      TString fDir;
      UInt_t fEntries;
      UInt_t fNbuf;

   public:
      ATMReader() : fEntries(0), fNbuf(0) {};
      virtual ~ATMReader() {};

      /**
       * Load index file of the binary file.
       */
      void LoadIndex(Int_t run, Int_t subrun, const char *dir="")
      {}

      /**
       * Return entries in the binary file.
       */
      UInt_t GetEntries() { return fEntries; }

      /**
       * Load the i-th entry in the binary file.
       */
      void GetEntry(UInt_t i=0) {}

      UShort_t Nbuf() { return fNbuf; }

      sort_atm* HitInBuffer(UShort_t buffer, UShort_t hit) { return 0; }

      ClassDef(ATMReader,1);
};

#endif
