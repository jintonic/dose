void d2p(Int_t run=11111; Int_t sub=1; char* dir="./";)
{
   // input
   TChain *ti = new TChain("t");
   ti->Add(Form("%s/atm_%06d.%06d.root",dir,run,sub));

   Hits hits;
   ti->SetBranchAddress("hits",&hits);

   // output
   TFile *output = new TFile(Form("hits_%06d.%06d.root",run,sub),"recreate");
   TTree *to = new TTree("t",Form("events in run %d, sub run %d", run, sub));
   to->Branch("hits",&hits);

   TH1F *hRCL = new TH1F("hRCL", "radius of center of light [cm]",45,0,45);
   TH1F *hNPE = new TH1F("hNPE", "total number of photoelectrons",1000,0,10000);

   // loop over entries in the input file
   UInt_t nevt = ti.GetEntries();
   cout<<nevt<<" entries to be processed..."<<endl;
   for (UInt_t ievt=0; ievt<nevt; ievt++) {
      ti.GetEntry(ievt);
      // loop over hits in an entry
      UShort_t nhit = hits.N();
      for (UShort_t ihit=0; ihit<nhit; ihit++) {
         hits[ihit].Reconstruct();
      }
      to->Fill();

      hRCL->Fill(hits.CenterOfLight().R());
      hNPE->Fill(hits.TotalNPEs());
   }
   output->Write();
   output->Close();
}
