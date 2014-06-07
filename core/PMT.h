#ifndef PMT_H
#define PMT_H

#include <TObject.h>

namespace NICE { class PMT; }

class NICE::PMT: public TObject
{
   public:
      enum Status {
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
       * - id == -10: TRG input
       * - id == -20: LED input
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

      PMT(): TObject(), st(kOk), id(-1), ch(-1), dt(0), gain(1) {};
      virtual ~PMT() {};

      void SetStatus(const char *status);
      const char* GetStatus() const;
      void Dump() const;

      ClassDef(PMT,1);
};

#endif
