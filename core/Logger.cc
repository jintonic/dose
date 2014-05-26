#include "Logger.h"

const int NICE::Logger::gVerbosity = 0;

int NICE::Logger::GetVerbosity()
{
   if (verbosity<=0) return gVerbosity;
   else return verbosity;
}
