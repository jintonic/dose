using namespace NICE;

void pe(int run=107)
{
   TChain *t = new TChain("t");
   t->Add(Form("~/data/nice/000100/run_%06d.000001.root",run));

   WFs *evt = new WFs;
   t->SetBranchAddress("evt", &evt);

   TFile *output = new TFile(Form("%d.root",run),"recreate");
   TH1D *h = new TH1D("h","",150,-50,100);
   for(int i=0; i<t->GetEntries(); i++) {
      t->GetEntry(i);
      if (evt->At(0)->ped==0) continue;
      double total=0;
      for (int j=170; j<200; j++) {
         total+=evt->At(0)->smpl[j];
      }
      h->Fill(total);
   }
   h->Write();
   output->Close();

   fit(run);
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
         "gaus + [3]*exp(-0.5*((x-[4])/[5])**2) + [6]*exp(-0.5*((x-2*[4])/[5])**2)",
         -50,100);
   f->SetParNames("norm0", "mean0", "sigma0", "norm1", "mean", "sigma", "norm2");
   f->SetParameters(3e4,0,4.8,67,28,10);
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

}


