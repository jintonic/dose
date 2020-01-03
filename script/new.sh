#!/bin/sh
# add new runs to run.sc

# get last run in run.sc
min=`tail -20 $NICESYS/script/run.sc | egrep "let A[0-9]+ = " | tail -1 | awk '{print $4}'`
# get last run in $NICEDAT
dir=`ls -1 $NICEDAT | egrep '^[0-9]+$' | tail -1`
max=`ls -1 $NICEDAT/$dir/run_??????.?????? | tail -1`
max=${max#*run_}
max=`echo ${max:0:6} | bc`
if [ $min -eq $max ]; then echo "Nothing to update, quit."; exit; fi

# loop over run [$min, $max]
run=$min
while [ $run -lt $max ]; do
  run=$((run+1))
  echo add run $run start and end times to run.sc

  # check how many sub runs are there
  r6d=`printf "%06d" $run`
  dir=${r6d:0:4}00
  nsub=`ls -1 $NICEDAT/$dir/run_${r6d}.??????.idx 2> /dev/null | wc -l`

  if [ $nsub -eq 0 ]; then
    echo run $run is not indexed, skipped
    continue;
  fi

  tmin=`hexdump -s 24 -n 4 -e '1 "%u"' $NICEDAT/$dir/run_$r6d.000001`
  s6d=`printf "%06d" $nsub`
  skip=`tail -1 $NICEDAT/$dir/run_$r6d.$s6d.idx | awk '{print $1}'`
  tmax=`hexdump -s $((skip+24)) -n 4 -e '1 "%u"' $NICEDAT/$dir/run_$r6d.$s6d`

  echo let A$run = $run  >> $NICESYS/script/run.sc
  echo let B$run = $tmin >> $NICESYS/script/run.sc
  echo let C$run = $tmax >> $NICESYS/script/run.sc
done
