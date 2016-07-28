using namespace NICE;

void pe(int run=119)
{
   TChain *t = new TChain("t");
   t->Add(Form("~/data/nice/%04d00/run_%06d.000001.root",run/100,run));

   WFs *evt = new WFs;
   t->SetBranchAddress("evt", &evt);
   t->GetEntry(1);

   TFile *output = new TFile(Form("%d.root",run),"recreate");
   TH1D *h = new TH1D("h","",200,-50,150);
   TH1D *hmin = new TH1D("hmin","",evt->nmax,0,evt->nmax);
   TH1D *hmax = new TH1D("hmax","",evt->nmax,0,evt->nmax);
   TH1D *hrange = new TH1D("hrange","",100,0,100);
   int nevt = t->GetEntries();
   for(int i=0; i<nevt; i++) {
      t->GetEntry(i);
      if (evt->At(0)->ped==0) continue;
      double total=0, threshold;
      int min=99999, max=0;
      getRange(evt,min,max, threshold=1.5);
      hmin->Fill(min);
      hmax->Fill(max);
      hrange->Fill(max-min);
      //if (min!=0) cout<<i<<", "<<min<<", "<<max<<endl;
      for (int j=min; j<max; j++) {
         total+=evt->At(0)->smpl[j];
      }
      h->Fill(total);
   }
   output->Write();
   output->Close();

   fit(run);
}

void getRange(WFs *evt, int &min, int &max, double threshold)
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

void fit(int run=107)
{
   TFile *file = new TFile(Form("%d.root",run));
   TH1D *h = (TH1D*) file->Get("h");
   h->GetXaxis()->SetTitle("ADC counts");
   h->GetYaxis()->SetTitle("Entries");

   TCanvas *c = new TCanvas;
   c->SetLogy();
   gStyle->SetOptFit(1);
   gStyle->SetOptStat(10);
   gStyle->SetStatW(0.2);
   gStyle->SetStatX(0.9);
   gStyle->SetStatY(0.9);

   TF1 *f = new TF1("f", 
         "gaus + [3]*exp(-0.5*((x-[4])/[5])**2) + [6]*exp(-0.5*((x-2*[4])/[5])**2) + [7]*exp(-0.5*((x-3*[4])/[5])**2)",
         -50,100);
   f->SetParNames("norm0", "mean0", "sigma0", "norm1", "mean", "sigma", "norm2", "norm3");
   f->SetParameter(1,0);
   f->SetParameter(2,2.6);
   f->SetParameter(4,31);
   f->SetParameter(5,8.5);
   h->Fit(f);

   TF1 *baseline = new TF1("baseline","gaus",-50,50);
   baseline->SetParameters(f->GetParameter(0),
         f->GetParameter(1), f->GetParameter(2));
   baseline->SetLineColor(kRed);
   baseline->Draw("same");

   TF1 *first = new TF1("first","gaus",-50,100);
   first->SetParameters(f->GetParameter(3),
         f->GetParameter(4), f->GetParameter(5));
   first->SetLineColor(kBlue);
   first->Draw("same");

   TF1 *second = new TF1("second","gaus",-50,100);
   second->SetParameters(f->GetParameter(6),
         2*f->GetParameter(4), f->GetParameter(5));
   second->SetLineColor(kGreen);
   second->Draw("same");

   TF1 *third = new TF1("third","gaus",-50,100);
   third->SetParameters(f->GetParameter(7),
         3*f->GetParameter(4), f->GetParameter(5));
   third->SetLineColor(kMagenta);
   third->Draw("same");
}


