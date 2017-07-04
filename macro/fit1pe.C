NICE::WFs *evt = new NICE::WFs; // data structure
const char *folder = gSystem->Getenv("NICEDAT"); // data location
int min=99999, max=0; // integration range (will be updated automatically)

// integration range is not fixed by default
void fit1pe(int run=112, int fixedMin=0, int fixedMax=0)
{
   if (fixedMin!=0 && fixedMax!=0) DrawWFs(run);
   Create1PEdistr(run, fixedMin, fixedMax);
   Fit1PEdistr(run);
}

void Create1PEdistr(int run, int fixedMin, int fixedMax)
{
   TChain *t = new TChain("t");
   t->Add(Form("%s/%04d00/run_%06d.000001.root",folder,run/100,run));

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
      if (i%5000==0) cout<<"now event "<<i<<endl;
      t->GetEntry(i);
      if (evt->At(0)->ped==0) continue;
      // search for proper range for integration
      if (fixedMin==0 && fixedMax==0) {
         double threshold=2;
         SearchPulseAboveThreshold(threshold);
         hmin->Fill(min);
         hmax->Fill(max);
         hrange->Fill(max-min);
      } else {
         min=fixedMin; max=fixedMax;
      }
      // skip events with fluctuating baseline
      double sum=0, sum1=0, sum2=0; int nb=10;
      for (int j=0; j<nb; j++) {
         sum1+=evt->At(0)->smpl[j];
         sum2+=evt->At(0)->smpl[evt->nmax-1-j];
      }
      sum=sum2-sum1;
      if (abs(sum)>5.7) continue;
      // integrate over [min, max)
      double total=0;
      for (int j=min; j<max; j++) {
         total+=evt->At(0)->smpl[j];
      }
      hpe->Fill(total);
   }
   output->Write();
   output->Close();
}

void Fit1PEdistr(int run)
{
   TFile *file = new TFile(Form("%d.root",run));
   TH1D *hpe = (TH1D*) file->Get("hpe");
   hpe->SetTitle(Form("run %d;ADC counts #bullet ns;Entries",run));

   TCanvas *can = new TCanvas;
   can->SetLogy();
   gStyle->SetOptFit(1);
   gStyle->SetOptStat(10);

   TF1 *f = new TF1("f", "gaus + gaus(3)"
         "+ [6]*exp(-0.5*((x-2*[4])/[5])**2)"
         " + [7]*exp(-0.5*((x-3*[4])/[5])**2)", -50,200);
   f->SetParNames("n0", "m0", "s0", "norm", "mean", "sigma", "n2", "n3");
   f->SetParameter(1,0);
   f->SetParameter(2,4);
   f->SetParameter(4,40);
   f->SetParameter(5,10);
   f->SetParLimits(5, 6, 16);
   f->SetParLimits(6, 0, 500);
   f->SetParLimits(7, 0, 500);
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

//______________________________________________________________________________
// different ways to specify the integration range [min, max]

// overlap many WFs in one plot to check location of 1 PE pulses by eyes
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

// search the range of the 1st pulse above threshold
void SearchPulseAboveThreshold(double threshold)
{
   for (int i=0; i<evt->nmax-2; i++) {
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
}

