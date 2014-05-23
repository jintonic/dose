/**
 * convert binary file to ROOT format.
 */
void b2r(Int_t run=11111; Int_t sub=1; char* dir="./";)
{
   CAEN_V1751 input(run, sub, dir);
   TFile *output = new TFile(Form("%s.root",input.file),"recreate");
   TTree *t = new TTree("t",Form("events in run %d, sub run %d", run, sub));
   t->Branch("wfs",&input);

   cout<<input.entries<<" entries to be processed"<<endl;
   for (UInt_t i=0; i<input.entries; i++) {
      input.GetEntry(i);
      t->Fill();
      if (i%5000==0) cout<<"entry "<<i<<" processed"
   }
   output->Write();
   output->Close(); // TTree is deleted here
   delete output;
   input.Close();
   cout<<"done"<<endl;
}
