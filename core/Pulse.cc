#include "iostream"
using namespace std;

#include <TClass.h>

#include "Pulse.h"
using namespace NICE;

//------------------------------------------------------------------------------

Pulse::Pulse(const Pulse& pls) : TObject(pls),
   bgn(pls.bgn),
   end(pls.end),
   ithr(pls.ithr),
   ih(pls.ih),
   h(pls.h),
   npe(pls.npe) {}

//------------------------------------------------------------------------------

Pulse& Pulse::operator=(const Pulse& pls)
{
   if (this==&pls) return *this;
   TObject::operator=(pls);
   bgn = pls.bgn;
   end = pls.end;
   ithr= pls.ithr;
   ih  = pls.ih;
   h   = pls.h;
   npe = pls.npe;
   return *this;
}
