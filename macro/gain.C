using namespace NICE;
// generate single PE distribution
void gain(const char *file="~Jing.Liu/data/nice/000000/run_000069.000001.root")
{
   // input
   TChain *t = new TChain("t");
   t->Add(file);
   WFs *wf = new WFs;
   t->SetBranchAddress("evt",&wf);

   // output
   TFile *f = new TFile("singlePE.root","recreate");
   TH1D *h = new TH1D("h", ";area [ADC counts];Entries/(1 ADC counts)", 350,-100,250);

   // main loop
   int nevt = t->GetEntries();
   cout<<nevt<<" events to process"<<endl;
   for(int i=0; i<nevt; i++) {
      t->GetEntry(i);
      if (i%10000==0) cout<<"processing event "<<i<<endl;
      if (wf->evt<0) continue; // skip none physical events
      h->Fill(wf->At(0)->GetIntegral(155,215));
   }

   h->Write();
   f->Close();
}
