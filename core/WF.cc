#include <float.h>

#include "WF.h"

bool NICE::WF::IsSimilarTo(const WF& other) const
{
   bool similar = samples.size()==other.samples.size() && 
      freq==other.freq && type==other.type;

   return similar;
}

//------------------------------------------------------------------------------

void NICE::WF::MakeSimilarTo(const WF& other)
{
   samples.resize(other.samples.size());
   freq = other.freq;
   type = other.type;
}

//------------------------------------------------------------------------------

NICE::WF& NICE::WF::operator+=(const WF& other)
{
   if (IsSimilarTo(other)==kFALSE) {
      Warning("operator+=", 
            "Only similar waveforms can be added together! Do nothing.");
      return *this;
   }

   for (size_t i=0; i<samples.size(); i++) samples[i] += other.samples[i];

   return *this;
}

//------------------------------------------------------------------------------

NICE::WF& NICE::WF::operator-=(const WF& other)
{
   if (IsSimilarTo(other)==kFALSE) {
      Warning("operator-=", 
            "Only a similar waveform can be subtracted! Do nothing.");
      return *this;
   }

   for (size_t i=0; i<samples.size(); i++) samples[i] -= other.samples[i];

   return *this;
}

//------------------------------------------------------------------------------

NICE::WF& NICE::WF::operator*=(const WF& other)
{
   if (IsSimilarTo(other)==kFALSE) {
      Warning("operator*=", 
            "Only similar waveforms can be multiplied! Do nothing.");
      return *this;
   }

   for (size_t i=0; i<samples.size(); i++) samples[i] *= other.samples[i];

   return *this;
}

//------------------------------------------------------------------------------

NICE::WF& NICE::WF::operator/=(const WF& other)
{
   if(!IsSimilarTo(other)) {
      return *this;
   }

   for (size_t i=0; i<samples.size(); i++)
      samples[i] /= (other.samples[i]==0.0) ? DBL_MIN : other.samples[i];

   return *this;
}

//------------------------------------------------------------------------------

NICE::WF& NICE::WF::operator+=(double value)
{
   for (size_t i=0; i<samples.size(); i++) samples[i] += value;

   return *this;
}

//------------------------------------------------------------------------------

NICE::WF& NICE::WF::operator*=(double value)
{
   for (size_t i=0; i<samples.size(); i++) samples[i] *= value;

   return *this;
}

//------------------------------------------------------------------------------

NICE::WF& NICE::WF::operator/=(double value) 
{ 
   if (value==0.0) {
      Warning("operator/", "Cannot be divided by zero! Do nothing.");
      return *this;
   }

   return (*this) *= 1.0/value;
}

//------------------------------------------------------------------------------
