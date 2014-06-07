#ifndef READER_H
#define READER_H

#include <fstream>
#include <vector>

#include <WFs.h>
#include <Logger.h>

class Reader: public NICE::WFs, public NICE::Logger
{
   private:
      ifstream *fRaw;
      TString fPath, fName;
      std::vector<size_t> fBegin;
      std::vector<size_t> fSize;

      /**
       * Event header defined by CAEN
       */
      typedef struct {
         unsigned evtSize        :28;
         unsigned begin          :4;
         unsigned chanMask       :8;
         unsigned pattern        :16;
         unsigned ZLE            :1; ///< always 0
         unsigned pack2_5        :1;
         unsigned res1           :1;
         unsigned boardID        :5;
         unsigned eventCount     :24;
         unsigned reserved2      :8; ///< 0: run header, 1: real data
         unsigned TTimeTag       :32;///< ticks of master clock
      } CAEN_DGTZ_LHEADER_t;

      /**
       * Run header defined in DAQ software
       */
      typedef struct {
         int run_number;
         int sub_run_number;
         int linux_time_sec;
         int linux_time_us;
         unsigned int custom_size;
         unsigned int pre_trigger;
         unsigned int nsamples_lbk;
         unsigned int nsamples_lfw;
         unsigned int threshold_under;
         unsigned int threshold_upper;
         unsigned int threshold_base;
         unsigned int timeout_base;
         unsigned int nsamples_base;
      } RUN_INFO;

   public:
      Reader(int run, int sub=1, const char *dir=".");
      virtual ~Reader() {};

      const char* GetFile() { return fName.Data(); }

      void Close() { if (fRaw) if (fRaw->is_open()) fRaw->close(); }

      void GetEntry(int i=0);

      void LoadIndex();
      void Index();
      void DumpIndex();

      void ReadRunInfo(int i);
      void ReadEvent(int i);
};

#endif
