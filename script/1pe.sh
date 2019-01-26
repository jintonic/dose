#!/bin/sh
# analyze all 1 PE runs

cat << _EOF_
Please select integration range:

1. Around the 1st peak above threshold
2. Fixed range as specified in *.1pe

_EOF_

read -p "Enter selection [1(default) or 2]: "
range="aroundPeak"
if [ X$REPLY != X ]; then
  if [ $REPLY -eq 2 ]; then range="fixed"; fi
fi

cat << _EOF_

Please select PMT:

1. R11065, 3-inch PMT
2. R8778(AR), 2-inch PMT

_EOF_

read -p "Enter selection [1(default) or 2]: "
PMT=R11065
if [ X$REPLY != X ]; then
  if [ $REPLY -eq 2 ]; then PMT="R8778(AR)"; fi
fi

echo
echo Currently available information for PMT $PMT:
last=`tail -1 $NICESYS/script/$PMT.1pe | awk '{print $1}'`
$NICESYS/script/run.sh
grep "$PMT" $NICESYS/script/run.txt | grep "1 PE" | awk -v lr=$last '
{ if ($1 > lr) printf("%4d %d %d %6.1f %d %2d\n", $1, $2, $5, $7, $10, $12) }
' >> $NICESYS/script/$PMT.1pe
cat $NICESYS/script/$PMT.1pe

read -p "Update run [<Enter> for all, 0 for none, or a specific run number]: "
runs=`awk '{if (NR>1) print $1}' $NICESYS/script/$PMT.1pe`
if [ X$REPLY != X ]; then # there is user input
  if [ $REPLY -eq 0 ]; then  # input = 0
    exit
  else # a run is specified
    inList="" # assume it is not in the 1 PE run list
    for each in $runs; do
      if [ $each -eq $REPLY ]; then inList=yes; break; fi
    done
    if [ "$inList" ]; then
      runs=$REPLY;
    else
      echo run $REPLY is not a 1 PE measurement, exit.
      exit
    fi
  fi
fi

echo Process run $runs ...
for run in $runs; do
  mkdir -p $run
  sed -r "/fit1pe/ s/[0-9]+,/$run,/" $NICESYS/macro/fit1pe.C > $run/fit1pe.C
  Ncol=`awk -v r=$run '($1==r){print NF}' $NICESYS/script/$PMT.1pe`
  if [ "$range" = "fixed" -a $Ncol -ge 8 ]; then
    min=`awk -v r=$run '$1==r{print $7}' $NICESYS/script/$PMT.1pe`
    max=`awk -v r=$run '$1==r{print $8}' $NICESYS/script/$PMT.1pe`
    sed -i -r "/fit1pe/ s/in=[0-9]+/in=$min/" $run/fit1pe.C
    sed -i -r "/fit1pe/ s/ax=[0-9]+/ax=$max/" $run/fit1pe.C
  fi
  if [ $Ncol -ge 10 ]; then
    mean=`awk -v r=$run '$1==r{print $9}' $NICESYS/script/$PMT.1pe`
    sigma=`awk -v r=$run '$1==r{print $10}' $NICESYS/script/$PMT.1pe`
    sed -i -r "/SetParNames/,/^$/ s/4,[0-9]+/4,$mean/" $run/fit1pe.C
    sed -i -r "/SetParNames/,/^$/ s/5,[0-9]+/5,$sigma/" $run/fit1pe.C
  fi
  qsub -N pe$run -V -cwd -e $run.log -o $run.log -b y root -b -q $run/fit1pe.C
done

while true; do
  squeue -u $USER
  njobs=`squeue -u $USER | egrep " pe[0-9]+ " | wc -l`
  if [ $njobs -eq 0 ]; then break; fi
  sleep 3
done

echo Generate PDF ...
nplots=`ls wf[0-9]*.ps 2>/dev/null | wc -w`
if [ $nplots -gt 0 ]; then gs -q -sDEVICE=pdfwrite -o wfs.pdf wf[0-9]*.ps; fi
nplots=`ls 1pe[0-9]*.ps 2>/dev/null | wc -w`
if [ $nplots -gt 0 ]; then gs -q -sDEVICE=pdfwrite -o 1pe.pdf 1pe[0-9]*.ps; fi
echo Done

echo Fitted mean and sigma from log file:
for run in $runs; do
  baseline=`tail $run.log | awk '$1==2{print +$3}'`
  mean=`tail $run.log | awk '$1==5{print +$3}'`
  sigma=`tail $run.log | awk '$1==6{print +$3}'`
  mean=`echo $mean - $baseline | bc`
  echo $run $mean $sigma
done

rm -fr *.ps [0-9]*/ *.root
