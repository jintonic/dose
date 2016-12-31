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
   cout.precision(16);
   t->GetEntry(0); double start=wf->sec, t0=wf->t;
   cout<<"Start time: "<<start<<" second."<<endl;
   t->GetEntry(n-1); double t1=wf->t;
   cout<<"Live time: "<<(t1-t0)*1e-9<<" seconds."<<endl;
}
