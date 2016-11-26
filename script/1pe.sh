#!/bin/sh
# analyze all 1 PE runs

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
if [ ! -f $NICESYS/script/run.txt ]; then $NICESYS/script/run.sh; fi
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
  sed -r "5 s/[0-9]+/$run/" $NICESYS/macro/pe.C > $run/pe.C
  Ncol=`awk -v r=$run '($1==r){print NF}' $NICESYS/script/$PMT.1pe`
  if [ $Ncol -ge 8 ]; then
    min=`awk -v r=$run '$1==r{print $7}' $NICESYS/script/$PMT.1pe`
    max=`awk -v r=$run '$1==r{print $8}' $NICESYS/script/$PMT.1pe`
    sed -i -r -e "8 s/[0-9]+/$min/" -e "8 s/[0-9]+;/$max;/" $run/pe.C
  fi
  if [ $Ncol -ge 10 ]; then
    mean=`awk -v r=$run '$1==r{print $9}' $NICESYS/script/$PMT.1pe`
    sigma=`awk -v r=$run '$1==r{print $10}' $NICESYS/script/$PMT.1pe`
    sed -i -r "100,120 s/4,[0-9]+\)/4,$mean\)/" $run/pe.C
    sed -i -r "100,120 s/5,[0-9]+\)/5,$sigma\)/" $run/pe.C
  fi
  qsub -N pe$run -V -cwd -e $run.log -o $run.log -b y root -b -q $run/pe.C
done

while true; do
  qstat
  nw=`qstat | grep qw | wc -l`
  nr=`qstat | grep " r " | wc -l`
  if [ $nw -eq 0 -a $nr -eq 0 ]; then break; fi
  sleep 3
done

echo Combining plots...
gs -q -sDEVICE=pdfwrite -o wfs.pdf wf[0-9]*.ps
gs -q -sDEVICE=pdfwrite -o 1pe.pdf 1pe[0-9]*.ps
echo Done

echo Fitted mean and sigma from log file:
for run in $runs; do
  baseline=`awk '$1==2{printf("%.1f",$3)}' $run.log`
  mean=`awk '$1==5{printf("%.1f",$3)}' $run.log`
  sigma=`awk '$1==6{printf("%.1f",$3)}' $run.log`
  mean=`echo $mean - $baseline|bc`
  echo $run $mean $sigma
done

rm -fr *.ps [0-9]*/ *.root
