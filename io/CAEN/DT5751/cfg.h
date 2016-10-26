#ifndef CFG_H
#define CFG_H

#include <stdio.h>
#include <stdint.h>

#ifndef Nch
#define Nch (4) ///< total number of channels per module
#endif

typedef enum {
   SOFTWARE_TRG,
   INTERNAL_TRG,
   EXTERNAL_TTL,
   EXTERNAL_NIM,
} TRG_MODE_t; ///< trigger mode

/**
 * Definiation of run header.
 *
 * sizeof(RUN_CFG_t) = 44 instead of 42 
 * because of padding added to satisfy alignment constrains:
 * http://stackoverflow.com/questions/119123/why-isnt-sizeof-for-a-struct-equal-to-the-sum-of-sizeof-of-each-member
 *
 * It is bad to design a not aligned struct, but it is too late to fix it
 * - many runs has been taken with this header.
 */
typedef struct {
   uint32_t run;
   uint16_t subrun;
   uint16_t trg;         ///< trg mode
   uint32_t tsec;        ///< time from OS in second
   uint32_t tus;         ///< time from OS in micro second
   uint32_t ns;          ///< number of samples in each wf
   uint8_t  post;        ///< percentage of wf after trg, 0 ~ 100
   uint8_t  mask;        ///< channel mask, bit 1: on, 0: off
   uint8_t  mode[Nch];   ///< CAEN_DGTZ_TriggerMode_t
   uint16_t thr[Nch];    ///< 0 ~ 2^10-1
   uint16_t offset[Nch]; ///< 16-bit DC offset
} RUN_CFG_t;             ///< run configurations

typedef struct {
   uint32_t size;    ///< size of header + size of data
   uint32_t evtCnt;  ///< event count
   uint32_t trgCnt;  ///< ticks of master clock (reset every 17s)
   uint8_t  type;    ///< 0: run info, 1: real event
   uint8_t  reserved;///< padding byte
   uint16_t reserved2;
} EVENT_HEADER_t;    ///< event header

void ConfigRunTime(RUN_CFG_t *cfg);

int ParseConfigFile(FILE *fcfg, RUN_CFG_t *cfg);

#endif
