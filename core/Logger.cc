#include "Logger.h"
using namespace NICE;

int Logger::gVerbosity = 0;

int Logger::GetVerbosity()
{
   if (verbosity<=0) return gVerbosity;
   else return verbosity;
}
