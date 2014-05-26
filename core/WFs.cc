#include "WFs.h"

//void NICE::WFs::LoadDatabase(const char* db)
//{
//   string dir; // database directory in type of string
//
//   // check whether database directory exists
//   struct stat st;
//   if (stat(db,&st) != 0) { // if not exist
//      string niceSys(gSystem->Getenv("NICESYS")); //check environment variable
//      dir = niceSys+"/db/pmt/";
//      if (stat(dir.c_str(),&st) != 0) { // if still not
//         Error("LoadData", "cannot open %s\n and %s\n", db, dir.c_str());
//         return;
//      }
//   } else { // if exists
//      if (*(dir.end()-1) != '/') dir+="/pmt/";
//      else dir+="pmt/";
//   }
//
//   // open database directory
//   struct dirent **list;
//   Int_t nPMTs = scandir(dir.c_str(), &list, 0, alphasort);
//   if (nPMTs<=0) {
//      Error("LoadData", "cannot open %s", dir.c_str());
//      return;
//   }
//   for (Int_t i=0; i<nPMTs; i++) {
//      string aPMT(list[i]->d_name);
//         if (aPMT[2]=='M') {// mapping data files
//            fIDMappingFileNames.push_back(dir+aPMT);
//            fIDMappingFileIds.push_back(atoi(aPMT.substr(10,6).c_str()));
//         } else if (aPMT[2]=='F') {// FADC ADC calibration data files
//            fIDFADCAdcFileNames.push_back(dir+aPMT);
//            fIDFADCAdcFileIds.push_back(atoi(aPMT.substr(10,6).c_str()));
//         } else if (aPMT[2]=='G') {// gain data files
//            fIDGainFileNames.push_back(dir+aPMT);
//            fIDGainFileIds.push_back(atoi(aPMT.substr(7,6).c_str()));
//         } else if (aPMT[2]=='S') {// status data files
//            fIDStatusFileNames.push_back(dir+aPMT);
//            fIDStatusFileIds.push_back(atoi(aPMT.substr(9,6).c_str()));
//         } else if (aPMT[2]=='d') {// dT data files
//            fIDdTFileNames.push_back(dir+aPMT);
//            fIDdTFileIds.push_back(atoi(aPMT.substr(5,6).c_str()));
//         } else if (aPMT[2]=='C') {// gain correction data files
//            fIDCorrectionFileNames.push_back(dir+aPMT);
//            fIDCorrectionFileIds.push_back(atoi(aPMT.substr(5,6).c_str()));
//         } else if (aPMT[2]=='A') {
//            if (aPMT[5]=='A') {// AtmAdc data files
//               fIDAtmAdcFileNames.push_back(dir+aPMT);
//               fIDAtmAdcFileIds.push_back(atoi(aPMT.substr(9,6).c_str()));
//            } else if (aPMT[5]=='T') {// AtmTdc data files
//               fIDAtmTdcFileNames.push_back(dir+aPMT);
//               fIDAtmTdcFileIds.push_back(atoi(aPMT.substr(9,6).c_str()));
//            }
//         } else if (aPMT[2]=='Y') { // light yield
//            if (aPMT[7]=='_') // bad PMT list from the last run is used
//               fInnerPMTs.LoadYieldDatabase((dir+aPMT).c_str(),"last run");
//            else // bad PMT list from the current run is used
//               fInnerPMTs.LoadYieldDatabase((dir+aPMT).c_str());
//         }
//      } else if (aPMT[0]=='A') { // ATM bad period list aPMT
//         fATMBadPeriodListFile = dir+aPMT;
//      }
//      free(list[i]);
//   }
//   free(list);
//}
//
////------------------------------------------------------------------------------
//
//PMT* NICE::WFs::Add(Int_t ch)
//{
//   for (Int_t i=0; i<fEntries; i++) {
//      if (channels[i]!=ch) continue;
//      Warning("Add", "try to add more than 1 wfs to channel %d",ch);
//      return 0;
//   }
//
//   // hard coded ch-pmt mapping table
//   Int_t pmtId = -1;
//   if (run<=999999) {
//      if (ch==0) pmtId = 0;
//      else return 0; // empty channel
//   }
//
//   PMT *aPMT = new PMT(pmtId);
//   return (PMT*) TClonesArray::Add(aPMT);
//}
