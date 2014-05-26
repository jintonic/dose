#ifndef ELECTRONICS_H
#define ELECTRONICS_H

namespace NICE { class Electronics; }

class NICE::Electronics
{
   public:
      enum Status {
         kDead,
         kNormal=0,
         kNoisy,
      };

      /**
       * Status of this channel
       */
      Status st; // status
      /**
       * Id of PMT connected to this channel
       */
      short id; // PMT id
      /**
       * Id of this channel
       * It equals to crate*Nmo*Nch + module*Nch + channel
       */
      short ch; // channel id

      Electronics(): st(kNormal), id(-1), ch(-1) {};
      virtual ~Electronics() {};
};

#endif
