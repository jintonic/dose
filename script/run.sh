#!/bin/sh
# convert run.sc to run.txt
awk '
BEGIN { isSetting=1; isHeader=0; FS="="; }
{
  if ($2 == " \"run\"") { 
    isSetting=0; 
    isHeader=1;
  }
  if (isSetting) {
    next;
  }

  if (isHeader) {
    printf("%s ", substr($2,2));
  }
  if ($2 == " \" comment\"") {
    FS=" ";
    isHeader=0;
  } 

  column=substr($2,1,1);
  if (column == "A") {
    printf("\n");
  }
  for (i=4; i<=NF; i++) {
    printf("%s ", $i);
  }
}
' $NICESYS/script/run.sc > $NICESYS/script/run.txt
