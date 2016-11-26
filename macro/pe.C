using namespace NICE;

const char *folder = gSystem->Getenv("NICEDAT");

void pe(int run=112)
{
   DrawWFs(run); // check location of 1 PE pulses
   int min=0, max=0; // integration range
   Create1PEdistr(run, min, max);
   Fit1PEdistr(run);
}

void DrawWFs(int run)
{
   TChain *t = new TChain("t");
   t->Add(Form("%s/%04d00/run_%06d.000001.root",folder,run/100,run));
   t->Draw("wf[0].smpl:Iteration$","","l",250,1);
   TText *text = new TText(.8,.8,Form("%d",run));
   text->SetNDC();
   text->Draw();
   gPad->Print(Form("wf%d.ps",run));
}

void Create1PEdistr(int run, int min, int max)
{
   TChain *t = new TChain("t");
   t->Add(Form("%s/%04d00/run_%06d.000001.root",folder,run/100,run));

   WFs *evt = new WFs;
   t->SetBranchAddress("evt", &evt);
   t->GetEntry(1);

   TFile *output = new TFile(Form("%d.root",run),"recreate");
   TH1D *hpe = new TH1D("hpe","",200,-50,150);
   TH1D *hmin = new TH1D("hmin","",evt->nmax,0,evt->nmax);
   TH1D *hmax = new TH1D("hmax","",evt->nmax,0,evt->nmax);
   TH1D *hrange = new TH1D("hrange","",100,0,100);
   int nevt = t->GetEntries();
   if (nevt>50000) nevt=50000; // no need to load more than this
   for(int i=0; i<nevt; i++) {
      t->GetEntry(i);
      if (evt->At(0)->ped==0) continue;
      // search for proper range if not specified
      if (min==0 && max==0) {
         int min=99999, max=0;
         double threshold=2;
         GetProperRange(evt,min,max, threshold);
         hmin->Fill(min);
         hmax->Fill(max);
         hrange->Fill(max-min);
      }
      double total=0;
      for (int j=min; j<max; j++) {
         total+=evt->At(0)->smpl[j];
      }
      hpe->Fill(total);
   }
   output->Write();
   output->Close();
}

void GetProperRange(WFs *evt, int &min, int &max, double threshold)
{
   for (int i=0; i<evt->nmax; i++) {
      if (  evt->At(0)->smpl[i]  >threshold && 
            evt->At(0)->smpl[i+1]>threshold && 
            evt->At(0)->smpl[i+2]>threshold) {
         min=i;
         break;
      }
   }
   if (min==99999) min=0;

   for (int i=min; i<evt->nmax; i++) {
      if (evt->At(0)->smpl[i]<threshold) {
         max=i;
         break;
      }
   }
   int middle = (min+max)/2;
   min= middle-20;
   max= middle+30;

   if (min<0) min=0;
   if (max-min<50) max=min+50;
   if (max>evt->nmax) max=evt->nmax;

   return;
}

void Fit1PEdistr(int run)
{
   TFile *file = new TFile(Form("%d.root",run));
   TH1D *hpe = (TH1D*) file->Get("hpe");
   hpe->SetTitle(Form("run %d;ADC counts;Entries",run));

   TCanvas *can = new TCanvas;
   can->SetLogy();
   gStyle->SetOptFit(1);
   gStyle->SetOptStat(10);

   TF1 *f = new TF1("f", "gaus + gaus(3)"
         "+ [6]*exp(-0.5*((x-2*[4])/[5])**2)"
         " + [7]*exp(-0.5*((x-3*[4])/[5])**2)", -50,200);
   f->SetParNames("n0", "m0", "s0", "norm", "mean", "sigma", "n2", "n3");
   f->SetParameter(1,0);
   f->SetParameter(2,6);
   f->SetParameter(4,40);
   f->SetParameter(5,10);
   hpe->Fit(f);

   TF1 *baseline = new TF1("baseline","gaus",-50,50);
   baseline->SetParameters(f->GetParameters());
   baseline->SetLineColor(kRed);
   baseline->Draw("same");

   TF1 *first = new TF1("first","gaus",-50,100);
   first->SetParameters(f->GetParameter(3),
         f->GetParameter(4), f->GetParameter(5));
   first->SetLineColor(kBlue);
   first->Draw("same");

   TF1 *second = new TF1("second","gaus",-50,170);
   second->SetParameters(f->GetParameter(6),
         2*f->GetParameter(4), f->GetParameter(5));
   second->SetLineColor(kGreen);
   second->Draw("same");

   TF1 *third = new TF1("third","gaus",-50,200);
   third->SetParameters(f->GetParameter(7),
         3*f->GetParameter(4), f->GetParameter(5));
   third->SetLineColor(kMagenta);
   third->Draw("same");

   can->Print(Form("1pe%d.ps",run));
}

