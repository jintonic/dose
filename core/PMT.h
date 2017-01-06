#ifndef PMT_H
#define PMT_H

#include <TObject.h>

namespace NICE { class PMT; }
/**
 * PMT information
 * It inherits Info(), Warning(), etc. functions from TObject to dump
 * information in terminal with <NICE::PMT::Function> prefixed.
 */
class NICE::PMT: public TObject
{
   public:
      enum Status {
         kUnknown,
         kDead,
         kFlashing,
         kNoisy,
         kOk,
         kAttenuated,
      };
      /**
       * PMT status
       */
      Status st; // PMT status
      /**
       * PMT id
       * loaded from $NICESYS/pmt/?/mapping.txt in WFs::Initialize()
       * - id >= 0: normal PMT
       * - id == -1: empty channel
       * - id == -10: coincidence
       * - id == -20: clock
       */
      short id; // PMT id
      /**
       * PMT channel number
       * It equals to crate*Nmo*Nch + module*Nch + channel,
       * where crate, module and channel all start from 0.
       */
      short ch; // PMT channel number
      /**
       * Time offset
       */
      double dt; // Time offset
      /**
       * Mean of 1 PE distribution
       */
      double gain; // mean of 1 PE distribution

      PMT();
      virtual ~PMT() {};

      void SetStatus(const char *status);
      const char* GetStatus() const;
      void Dump() const;

      ClassDef(PMT,1);
};

#endif
