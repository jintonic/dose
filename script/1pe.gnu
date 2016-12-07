#!/usr/bin/gnuplot -persist
# run it as $ gnuplot 1pe.gnu < R11065.1pe
set title "Stability of single PE distribution"

set xdata time
set timefmt "%s"
set format x "%b %d\n%Y"

set yr [20:45]
set ylabel 'Mean of single PE distribution [ADC x ns]'

set key left top Left reverse title "Voltage applied:"

set terminal png
set output "1pe.png"
plot "/dev/stdin" using 2:($5==1500?$9:1/0) title '1500 V' with points 4,\
 "" using 2:($5==1300?$9:1/0) title '1300 V (x4)' with points 8,\
 "" using 2:($5==1080?$9:1/0) title '1080 V (x10)' with points 10
set output
