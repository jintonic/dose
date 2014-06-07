#include <cstdio>
using namespace std;

#include <UNIC/Units.h>
using namespace UNIC;

#include "PMT.h"

void NICE::PMT::SetStatus(const char* status)
{
   if (status[0]=='d') st = kDead;
   else if (status[0]=='f') st = kFlashing;
   else if (status[0]=='n') st = kNoisy;
   else if (status[0]=='o') st = kOk;
   else if (status[0]=='a') st = kAttenuated;
   else Warning("SetStatus", "status, %s, is unknown", status);
}

//------------------------------------------------------------------------------

const char* NICE::PMT::GetStatus() const
{
   if (st==kDead) return "dead";
   else if (st==kFlashing) return "flashing";
   else if (st==kNoisy) return "noisy";
   else if (st==kOk) return "ok";
   else if (st==kAttenuated) return "attenuated";
   else return "unknown";
}

//------------------------------------------------------------------------------

void NICE::PMT::Dump() const
{
   if (id>=0) Info("Dump", "ch %2d: PMT %2d (gain =%4.1f, dt =%6.1f*ns, %s)", 
         ch, id, gain, dt/ns, GetStatus());
   else if (id==-1) Info("Dump", "ch %2d: empty", ch);
   else if (id==-10) Info("Dump", "ch %2d: TRG input", ch);
   else if (id==-20) Info("Dump", "ch %2d: LED input", ch);
   else Info("Dump", "ch %2d: unknown", ch);
}

//------------------------------------------------------------------------------

