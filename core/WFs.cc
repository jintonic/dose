#include <cstdlib>
#include <fstream>
using namespace std;

#include <dirent.h>
#include <sys/stat.h>

#include <TSystem.h>

#include <UNIC/Units.h>
using namespace UNIC;

#include "WF.h"
#include "WFs.h"
using namespace NICE;

WFs::WFs(int run) : TObject(), run(run), sub(-1), evt(-1), cnt(0), sec(0),
   nch(8), nmax(0), nfw(0), nbw(0), thr(0)
{
   wfs.SetClass("NICE::WF",nch);
}

//------------------------------------------------------------------------------

void WFs::Initialize(const char* db)
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

      WF *wf = Map(atoi(ch[i]->d_name));
      if (wf) {
         if (wf->pmt.id>=0) {
            LoadTimeOffset(wf);
            LoadMeanOf1PE(wf);
            LoadStatus(wf);
         }
         wf->pmt.Dump();
      }

      free(ch[i]);
   }
   free(ch);
}

//------------------------------------------------------------------------------

WF* WFs::Map(int ch)
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

   WF *wf = 0;
   Int_t runnum, id;
   while (file>>runnum>>id) {
      if (run<runnum) continue;
      wf = (WF*) wfs.ConstructedAt(ch);
      break;
   }

   file.close();

   if (!wf) {
      Error("Map", "no PMT mapped to ch %d in run %d", ch, run);
      Error("Map", "return 0");
      return 0;
   }

   wf->pmt.ch = ch;
   wf->pmt.id = id;
   return wf;
}

//------------------------------------------------------------------------------

void WFs::LoadTimeOffset(WF* wf)
{
   if (!wf) {
      Error("LoadTimeOffset", "NULL pointer to PMT, return");
      return;
   }

   ifstream file(Form("%s/%d/dt.txt", fDB.Data(), wf->pmt.ch), ios::in);
   if (!(file.is_open())) {
      Error("LoadTimeOffset", "cannot read %s/%d/dt.txt", fDB.Data(), wf->pmt.ch);
      Error("LoadTimeOffset", "return");
      return;
   }

   Int_t runnum;
   double dt;
   while (file>>runnum>>dt) {
      if (run<runnum) continue;
      wf->pmt.dt = dt*ns;
      break;
   }

   file.close();
}

//------------------------------------------------------------------------------

void WFs::LoadMeanOf1PE(WF* wf)
{
   if (!wf) {
      Error("LoadMeanOf1PE", "NULL pointer to PMT, return");
      return;
   }

   ifstream file(Form("%s/%d/gain.txt", fDB.Data(), wf->pmt.ch), ios::in);
   if (!(file.is_open())) {
      Error("LoadMeanOf1PE", "cannot read %s/%d/gain.txt", fDB.Data(), wf->pmt.ch);
      Error("LoadMeanOf1PE", "return");
      return;
   }

   Int_t runnum;
   double gain;
   while (file>>runnum>>gain) {
      if (run<runnum) continue;
      wf->pmt.gain = gain;
      break;
   }

   file.close();
}

//------------------------------------------------------------------------------

void WFs::LoadStatus(WF* wf)
{
   if (!wf) {
      Error("LoadStatus", "NULL pointer to PMT, return");
      return;
   }

   ifstream file(Form("%s/%d/status.txt", fDB.Data(), wf->pmt.ch), ios::in);
   if (!(file.is_open())) {
      Error("LoadStatus", "cannot read %s/%d/status.txt", fDB.Data(), wf->pmt.ch);
      Error("LoadStatus", "return");
      return;
   }

   Int_t runnum;
   TString st;
   while (file>>runnum>>st) {
      if (run<runnum) continue;
      wf->pmt.SetStatus(st);
      break;
   }

   file.close();
}

//------------------------------------------------------------------------------

WF* WFs::At(unsigned short i) const
{
   if (i>=wfs.GetEntries()) {
      Warning("operator[]", 
            "index %d >= %d (max), return 0", i, wfs.GetEntries());
      return 0;
   }
   return (WF*) wfs.At(i);
}

//------------------------------------------------------------------------------

