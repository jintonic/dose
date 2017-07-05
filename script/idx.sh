#!/bin/sh
# index all runs

# figure out valid range of runs, [min, max]
dir0=`ls -1 $NICEDAT | egrep '^[0-9]+$' | head -1`
dir1=`ls -1 $NICEDAT | egrep '^[0-9]+$' | tail -1`
min=`ls -1 $NICEDAT/$dir0/run_??????.?????? | head -1`
max=`ls -1 $NICEDAT/$dir1/run_??????.?????? | tail -1`
min=${min#*run_}
max=${max#*run_}
min=`echo ${min:0:6} | bc`
max=`echo ${max:0:6} | bc`
echo "Run ranges from $min to $max"

# skip old runs unless specified otherwise
min=245
read -p "Please set minimal run number (<Enter> to accept default: $min): " run
if [ X$run = X ]; then run=$min; fi

echo "Loop over run [$run,  $max]..."
while [ $run -le $max ]; do
  # specify executable
  exe=$NICESYS/daq/CAEN/DT5751/idx.exe
  if [ $run -lt 107 ]; then exe=$NICESYS/daq/CAEN/V1751/idx.exe; fi
  # loop over sub runs
  r6d=`printf "%06d" $run`
  dir=${r6d:0:4}00
  for file in `ls -1 $NICEDAT/$dir/run_$r6d.??????`; do
    # skip existing, valid idx files
    if [ -f $file.idx ]; then
      size=`tail -1 $file.idx | awk '{print $2}'`
      if [ X$size != X ]; then
	if [ $size -eq 68 -o $size -eq 60 ]; then continue; fi
      fi
    fi
    # submit jobs for necessary runs
    sub=`echo $file | awk -F. '{printf("%d",$NF)}'`
    qsub -N idx$run.$sub -V -cwd -o $file.idx -e run_${r6d}_`printf "%06d" $sub`.log -b y $exe $run $sub $NICEDAT
  done
  # next run
  run=$((run+1))
done

while true; do
  qstat
  njobs=`qstat | egrep " idx[0-9]+." | wc -l`
  if [ $njobs -eq 0 ]; then break; fi
  sleep 3
done
