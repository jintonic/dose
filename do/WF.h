#ifndef WF_H
#define WF_H

#include <vector>
#include <TObject.h>

#include "Electronics.h"

namespace NICE { class WF; }

class NICE::WF : public Electronics, public TObject
{
   public:
      enum Type {
         kCalibrated,
         kRaw,
      };
      Type type; 

      double freq;

      std::vector<double> sample;

      WF(): Electronics(), TObject(), type(kRaw), freq(0) {};
      virtual ~WF() {};

      bool IsSimilarTo(const WF& other) const;
      void MakeSimilarTo(const WF& other);

      WF& operator+=(const WF& other); 
      WF& operator-=(const WF& other); 
      WF& operator*=(const WF& other); 
      WF& operator/=(const WF& other); 

      WF& operator+=(double value); 
      WF& operator-=(double value) { return (*this) += -value; }
      WF& operator*=(double value); 
      WF& operator/=(double value);

      ClassDef(WF,1);
};

#endif
