using namespace NICE;
void runinfo(int run=1, int sub=1)
{
   const char *dir = gSystem->Getenv("NICEDAT");
   TChain *t = new TChain("t");
   t->Add(Form("%s/%04d00/run_%06d.%06d.root",dir,run/100,run,sub));

   WFs *wf = new WFs;
   t->SetBranchAddress("evt",&wf);
   
   int n = t->GetEntries();
   cout<<"Run "<<run<<" has "<<n<<" events"<<endl;
   cout.precision(26);
   t->GetEntry(0); double start=wf->t0, t0=wf->t;
   cout<<"Start time: "<<start<<" second."<<endl;
   t->GetEntry(n-1);
   cout<<"End time: "<<wf->t<<" second."<<endl;
   t->GetEntry(n-1); double t1=wf->t, dt = (t1-t0)*1e-9;
   cout<<"Live time: "<<dt<<" seconds."<<endl;

   t->Draw(Form("t>>h(100,0,%f)", t1-t0));
   TH1F* h = (TH1F*) gPad->GetPrimitive("h");
   if (h) {
      h->Scale(100e9/(t1-t0));
      h->SetTitle(";Time [ns];Trigger rate [Hz]");
      h->Fit("pol0");
   }
}
