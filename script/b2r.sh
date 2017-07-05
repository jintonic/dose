#!/bin/sh
# convert a range of binary files to root files

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
old=`ls -1 $NICEDAT/$dir1/run_??????.??????.root | tail -1`
old=${old#*run_}
old=`echo ${old:0:6} | bc`
echo "Please set minimal run number (<Enter> to accept default: $old):"
read -e -n ${#max} input
if [ "X$input" != X ]; then # not <Enter>
  while true; do # display promp until a valid input
    if [[ "$input" =~ ^[0-9]+$ ]]; then # positive integer
      if [ $input -le $max -a $input -ge $min ]; then break; fi
    fi
    echo; echo "Invalid run number: $input,"
    echo "please input an integer in [$min, $max]:"
    read -e -n ${#max} input
  done
  min=$input
else
  min=$old
fi

# specify max run number
echo "Please set maximal run number (<Enter> to accept default: $max):"
read -e -n ${#max} input
if [ "X$input" != X ]; then # not <Enter>
  while true; do # display promp until a valid input
    if [[ "$input" =~ ^[0-9]+$ ]]; then # positive integer
      if [ $input -le $max -a $input -ge $min ]; then break; fi
    fi
    echo; echo "Invalid run number: $input,"
    echo "please input an integer in [$min, $max]:"
    read -e -n ${#max} input
  done
  max=$input
fi

read -e -n1 -p "Overwrite existing root files? (y/N): " overwrite
if [ X$overwrite = X ]; then overwrite=N; fi

# threshold for peak searching
threshold=3
echo "Please set threshold for peak searching:"
echo "(<Enter> to accept default: $threshold ADC counts)"
read -e -n4 input
if [ "X$input" != X ]; then # not <Enter>
  while true; do # display promp until a valid input
    if [[ "$input" =~ ^[0-9]+$ ]]; then # positive integer
      if [ $input -lt 1024 ]; then break; fi
    fi
    echo; echo "Invalid threshold: $input,"
    echo "please input an integer in [1, 1024]:"
    read -e -n4 input
  done
  threshold=$input
fi

echo "Loop over run [$min, $max]..."
run=$min
while [ $run -le $max ]; do
  # specify executable
  if [ $run -lt 107 ]; then
    exe=$NICESYS/daq/CAEN/V1751-106/b2r.exe
  elif [ $run -lt 177 ]; then
    exe=$NICESYS/daq/CAEN/DT5751-176/b2r.exe
  else
    exe=$NICESYS/daq/CAEN/DT5751/b2r.exe
  fi
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
    rm -f $file.log
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
