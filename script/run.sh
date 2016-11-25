#!/bin/sh
# convert run.sc to run.txt
awk '
BEGIN { isSetting=1; isHeader=0; FS="="; }
{
  if ($2 == " \"run\"") { 
    isSetting=0; 
    isHeader=1;
  }
  if (isSetting) next;
  if (isHeader) printf("%s ", substr($2,2));

  if ($2 == " \" comment\"") {
    FS=" ";
    isHeader=0;
  } 

  column=substr($2,1,1);
  if (column == "A") printf("\n %3d ", $4);
  else if (column == "D") printf("%9s ", $4);
  else if (column == "E") printf("%6s ", $4);
  else if (column == "F") printf("%9s ", $4);
  else if (column == "G") printf("%6.1f ", $4);
  else if (column == "H") printf("%5s ", $4);
  else if (column == "I") printf("%11s ", $4);
  else if (column == "J") printf("%4d ", $4);
  else if (column == "K") printf("%11s ", $4);
  else if (column == "L") printf("%3d ", $4);
  else for (i=4; i<=NF; i++) printf("%s ", $i);
}
' $NICESYS/script/run.sc > $NICESYS/script/run.txt
sed -i 's/"//g' $NICESYS/script/run.txt
