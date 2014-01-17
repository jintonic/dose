#ifndef FADC_H
#define FADC_H

namespace OOPA {
   class FADC;
   enum FADCStatus {
      kNoisy,
      kNormal=0,
   }
}

class OOPA::FADC
{
   protected:
      UShort_t fFADCId;
      FADCStatus fFADCStatus;

   public:
      FADC() {};
      virtual ~FADC() {};

      ClassDef(FADC,1);
};

#endif
