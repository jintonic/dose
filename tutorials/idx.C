/**
 * index a binary file and dump to stdout
 *
 * format:
 * - one line one event
 * - each line: start length
 */
void idx(Int_t run=11111; Int_t sub=1; char* dir="./";)
{
   CAEN_V1751 input(run, sub, dir);
   input.DumpIndex();
   input.Close();
}
