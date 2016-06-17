using namespace NICE;
void runinfo(int run=1, int sub=1)
{
   const char *dir = gSystem->Getenv("NICEDAT");
   TChain *t = new TChain("t");
   t->Add(Form("%s/%06d/run_%06d.%06d.root",dir,run/100,run,sub));

   WFs *wf = new WFs;
   t->SetBranchAddress("evt",&wf);
   
   int n = t->GetEntries();
   cout<<"Run "<<run<<" has "<<n<<" events"<<endl;
   t->GetEntry(1);
   cout<<"It starts at "<<wf->sec<<endl;
   t->GetEntry(n-1);
   cout<<"     ends at "<<wf->sec<<endl;
}
