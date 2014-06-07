#include <cstdlib>
#include <fstream>
using namespace std;

#include <dirent.h>
#include <sys/stat.h>

#include <TSystem.h>

#include <UNIC/Units.h>
using namespace UNIC;

#include "WFs.h"

void NICE::WFs::Initialize(const char* db)
{
   fDB=db;
   struct stat st;
   if (stat(db,&st)!=0) { // if not exist
      Info("Initialize", "cannot open %s", db);
      Info("Initialize", "check $NICESYS");
      const char *root = gSystem->Getenv("NICESYS");
      if (root==0) {
         Warning("Initialize", "$NICESYS is not set, return");
         return;
      }
      Info("Initialize", "$NICESYS=%s",root);
      fDB=root;

      if (fDB.EndsWith("/")) fDB+="pmt/";
      else fDB+="/pmt/";
   }

   Info("Initialize", "scan %s", fDB.Data());
   struct dirent **ch;
   Int_t n = scandir(fDB.Data(), &ch, 0, alphasort);
   if (n<0) {
      Warning("Initialize", "cannot open %s", fDB.Data());
      Warning("Initialize", "return");
      return;
   }
   for (Int_t i=0; i<n; i++) {
      if (ch[i]->d_name[0]=='.' || i>=nch+2) {
         free(ch[i]);
         continue;
      }

      PMT *aPMT = Map(atoi(ch[i]->d_name));
      if (aPMT && aPMT->id>=0) {
         LoadTimeOffset(aPMT);
         LoadMeanOf1PE(aPMT);
         LoadStatus(aPMT);
      }
      aPMT->Dump();

      free(ch[i]);
   }
   free(ch);
}

//------------------------------------------------------------------------------

NICE::PMT* NICE::WFs::Map(int ch)
{
   if (ch<0) {
      Error("Map", "cannot handle channel number %d", ch);
      Error("Map", "return 0");
      return 0;
   }

   ifstream file(Form("%s/%d/mapping.txt", fDB.Data(), ch), ios::in);
   if (!(file.is_open())) {
      Error("Map", "cannot read %s/%d/mapping.txt", fDB.Data(), ch);
      Error("Map", "return 0");
      return 0;
   }

   PMT *aPMT = 0;
   Int_t runnum, id;
   while (file>>runnum>>id) {
      if (run<runnum) continue;
      aPMT = (PMT*) ConstructedAt(ch);
      break;
   }

   file.close();

   if (!aPMT) {
      Error("Map", "no PMT mapped to ch %d in run %d", ch, run);
      Error("Map", "return 0");
      return 0;
   }

   aPMT->ch = ch;
   aPMT->id = id;
   return aPMT;
}

//------------------------------------------------------------------------------

void NICE::WFs::LoadTimeOffset(PMT* pmt)
{
   if (!pmt) {
      Error("LoadTimeOffset", "NULL pointer to PMT, return");
      return;
   }

   ifstream file(Form("%s/%d/dt.txt", fDB.Data(), pmt->ch), ios::in);
   if (!(file.is_open())) {
      Error("LoadTimeOffset", "cannot read %s/%d/dt.txt", fDB.Data(), pmt->ch);
      Error("LoadTimeOffset", "return");
      return;
   }

   Int_t runnum;
   double dt;
   while (file>>runnum>>dt) {
      if (run<runnum) continue;
      pmt->dt = dt*ns;
      break;
   }

   file.close();
}

//------------------------------------------------------------------------------

void NICE::WFs::LoadMeanOf1PE(PMT* pmt)
{
   if (!pmt) {
      Error("LoadMeanOf1PE", "NULL pointer to PMT, return");
      return;
   }

   ifstream file(Form("%s/%d/gain.txt", fDB.Data(), pmt->ch), ios::in);
   if (!(file.is_open())) {
      Error("LoadMeanOf1PE", "cannot read %s/%d/gain.txt", fDB.Data(), pmt->ch);
      Error("LoadMeanOf1PE", "return");
      return;
   }

   Int_t runnum;
   double gain;
   while (file>>runnum>>gain) {
      if (run<runnum) continue;
      pmt->gain = gain;
      break;
   }

   file.close();
}

//------------------------------------------------------------------------------

void NICE::WFs::LoadStatus(PMT* pmt)
{
   if (!pmt) {
      Error("LoadStatus", "NULL pointer to PMT, return");
      return;
   }

   ifstream file(Form("%s/%d/status.txt", fDB.Data(), pmt->ch), ios::in);
   if (!(file.is_open())) {
      Error("LoadStatus", "cannot read %s/%d/status.txt", fDB.Data(), pmt->ch);
      Error("LoadStatus", "return");
      return;
   }

   Int_t runnum;
   TString st;
   while (file>>runnum>>st) {
      if (run<runnum) continue;
      pmt->SetStatus(st);
      break;
   }

   file.close();
}

//------------------------------------------------------------------------------

