void fit1pe(const char *input="singlePE.root")
{
   TFile *file = new TFile(input);
   TH1D *h = (TH1D*) file->Get("h");

   TF1 *f = new TF1("f", "gaus(0) + [3]*exp(-(x-[4])*(x-[4])/2/[5]/[5]) + [6]*exp(-(x-2*[4])*(x-2*[4])/2/[5]/[5])", -100,250);
   f->SetParNames("norm0", "mean0", "sigma0", "norm1", "mean", "sigma", "norm2");
   f->SetParameters(3118,-3.5,14, 166,55,30, 10);

   gStyle->SetOptFit(1);
   gStyle->SetOptStat(10);
   gStyle->SetStatH(0.12);
   TCanvas *c = new TCanvas;
   c->SetLogy();

   h->Fit(f);

   TF1 *g1 = new TF1("g1","gaus(0)",-100,100);
   g1->SetParameters(f->GetParameter(0), f->GetParameter(1), f->GetParameter(2));
   g1->SetLineColor(kBlue);
   g1->Draw("same");

   TF1 *g2 = new TF1("g2","gaus(0)",-100,200);
   g2->SetParameters(f->GetParameter(3), 1*f->GetParameter(4), f->GetParameter(5));
   g2->SetLineColor(kRed);
   g2->Draw("same");

   TF1 *g3 = new TF1("g3","gaus(0)",0,200);
   g3->SetParameters(f->GetParameter(6), 2*f->GetParameter(4), f->GetParameter(5));
   g3->SetLineColor(kGreen);
   g3->Draw("same");
}
