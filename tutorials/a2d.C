void a2d(Int_t run=11111; Int_t sub=1; char* dir="./";)
{
   Hits hits;

   // input
   ATMReader atm;
   atm.LoadIndex(run, sub, dir); // load index of the binary file

   // output
   TFile *output = new TFile(Form("atm_%06d.%06d.root",run,sub),"recreate");
   TTree *t = new TTree("t",Form("events in run %d, sub run %d", run, sub));
   t->Branch("hits",&hits);

   // histograms filled with very basic information
   TH1F *hNHits = new TH1F("hNHits", "Number of ATM hits",PMT::fgNPMTs,0,PMT::fgNPMTs);
   TH1F *hHitMap = new TH1F("hHitMap", "Hit map",PMT::fgNPMTs,0,PMT::fgNPMTs);

   // loop over entries in the binary file
   UInt_t nevt = atm.GetEntries();
   cout<<nevt<<" entries to be processed..."<<endl;
   for (UInt_t ievt=0; ievt<nevt; ievt++) {
      atm.GetEntry(ievt);
      // loop over atm buffers
      UShort_t nbuf = atm.Nbuf();
      for (UShort_t ibuf=0; ibuf<nbuf; ibuf++) {
         // loop over hits in a buffer
         UShort_t nhit = atm.NHitsInBuffer(ibuf);
         for (UShort_t ihit=0; ihit<nhit; ihit++) {
            // call ATMReader::Add(Hit *hit)
            // it overwrites Hits::Add() and does the following:
            // - atm.LoadPMTATMCableMapping()
            // - check if ATM channel is connected with a PMT
            // - decode sort_atm[ihit]
            ATMHit *hit = hits.Add(atm.HitInBuffer(ibuf, ihit));
            if (hit) hHitMap->Fill(hit->PMTId());
         }
         hNHits->Fill(atm.N());
      }
      t->Fill();
   }
   output->Write();
   output->Close();
}
