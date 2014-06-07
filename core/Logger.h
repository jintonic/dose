#ifndef LOGGER_H
#define LOGGER_H

namespace NICE { class Logger; }

class NICE::Logger
{
   public:
      static int gVerbosity; ///< global verbosity
      int verbosity; ///< local verbosity

      Logger(): verbosity(0) {};
      virtual ~Logger() {};

      int GetVerbosity();
};

#endif
