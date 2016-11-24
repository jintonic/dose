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
min=162
read -p "Please set minimal run number (<Enter> to accept default: $min): " run
if [ X$run = X ]; then run=$min; fi
read -p "Overwrite existing root files? (y/N): " overwrite
if [ X$overwrite = X ]; then overwrite=N; fi
echo $overwrite

echo "Loop over run [$run, $max]..."
while [ $run -le $max ]; do
  # specify executable
  exe=$NICESYS/io/CAEN/DT5751/b2r.exe
  if [ $run -lt 107 ]; then exe=$NICESYS/io/CAEN/V1751/b2r.exe; fi
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
    qsub -N b2r$run.$sub -V -cwd -o $file.log -e $file.log -b y $exe $run $sub $NICEDAT
  done
  # next run
  run=$((run+1))
done

qstat
