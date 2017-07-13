#include "iostream"
using namespace std;

#include <TClass.h>

#include "Pulse.h"
using namespace NICE;

Pulse::Pulse() : TObject(), bgn(0), end(0), ithr(0), ih(0), h(0), npe(0)
{ Class()->IgnoreTObjectStreamer(); }

//------------------------------------------------------------------------------

Pulse::Pulse(const Pulse& pls) : TObject(pls),
   bgn(pls.bgn),
   end(pls.end),
   ithr(pls.ithr),
   ih(pls.ih),
   h(pls.h),
   npe(pls.npe)
{ Class()->IgnoreTObjectStreamer(); }

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
