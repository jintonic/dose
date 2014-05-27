#ifndef PMT_H
#define PMT_H

namespace NICE { class PMT; }

class NICE::PMT
{
   public:
      enum Status {
         kDead,
         kFlashing,
         kNoisy,
         kNormal=0,
         kHighGain,
      };

      /**
       * PMT status
       */
      Status st; // PMT status
      /**
       * PMT id
       * Normal PMT id starts from 0.
       * id == -1: empty channel
       * id == -10: TRG input
       * id == -20; LED input
       */
      short id; // PMT id
      /**
       * PMT channel number
       * It equals to crate*Nmo*Nch + module*Nch + channel,
       * where crate, module and channel all start from 0.
       */
      short ch; // PMT channel number

      PMT(): st(kNormal), id(-1), ch(-1) {};
      virtual ~PMT() {};
};

#endif
