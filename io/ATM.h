#ifndef ATM_H
#define ATM_H

namespace OOPA {
   class ATM;
   enum ATMStatus {
      kNoisy,
      kNormal=0,
   }
}

class OOPA::ATM
{
   protected:
      UShort_t fATMId;
      ATMStatus fATMStatus;

   public:
      ATM() {};
      virtual ~ATM() {};

      ClassDef(ATM,1);
};

#endif
