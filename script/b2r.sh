#!/bin/sh
# convert all binary files to root files

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
min=171
read -p "Please set minimal run number (<Enter> to accept default: $min): " run
if [ X$run = X ]; then run=$min; fi
read -p "Overwrite existing root files? (y/N): " overwrite
if [ X$overwrite = X ]; then overwrite=N; fi
echo $overwrite

# threshold for peak searching
thr=3
echo "Please set threshold for peak searching:"
read -p "(<Enter> to accept default: $thr ADC counts) " threshold
if [ X$threshold = X ]; then threshold=$thr; fi

echo "Loop over run [$run, $max]..."
while [ $run -le $max ]; do
  # specify executable
  exe=$NICESYS/daq/CAEN/DT5751/b2r.exe
  if [ $run -lt 107 ]; then exe=$NICESYS/daq/CAEN/V1751/b2r.exe; fi
  # loop over sub runs
  r6d=`printf "%06d" $run`
  dir=${r6d:0:4}00
  for file in `ls -1 $NICEDAT/$dir/run_$r6d.??????`; do
    # skip existing, valid root files
    if [ $overwrite = "N" -a -f $file.log ]; then 
      convert=`tail -1 $file.log`
      if [ "$convert" = "done" ]; then continue; fi
    fi
    # generate root file
    cd $NICEDAT/$dir
    s6d=`echo $file | awk -F. '{print $NF}'`
    sub=`echo $file | awk -F. '{printf("%d",$NF)}'`
    qsub -N b2r$run.$sub -V -cwd -o $file.log -e $file.log -b y $exe $run $sub $NICEDAT $threshold 
  done
  # next run
  run=$((run+1))
done

while true; do
  qstat
  njobs=`qstat | egrep " b2r[0-9]+." | wc -l`
  if [ $njobs -eq 0 ]; then break; fi
  sleep 3
done
