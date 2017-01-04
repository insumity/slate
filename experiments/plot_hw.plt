#!/usr/bin/gnuplot
#clear
#reset
unset key
# Make the x axis labels easier to read.
set xtics rotate out
# Select histogram data
set style data histogram
# Give the bars a plain fill pattern, and draw a solid line around them.
set style fill solid border
set style histogram errorbars gap 1 lw 1
set key autotitle columnheader
set boxwidth 1 relative
set ylabel yaxislabel
set xlabel "Applications"
set yrange [0:*]
set mxtics
set mytics
set style line 12 lc rgb '#ddccdd' lt 1 lw 1.5
set style line 13 lc rgb '#ddccdd' lt 1 lw 0.5
set grid xtics mxtics ytics mytics back ls 12, ls 13
set terminal pngcairo
set output result_file

plot filename using 3:6:xtic(1) title 'slate-IPC', '' using 9:12:xtic(1) title 'Linux-IPC', '' using 4:7:xtic(1) title 'slate-LLC', '' using 10:13:xtic(1) title 'Linux-LLC'



#plot for [COL=from:to] filename using COL:8:xticlabels(1) title columnheader
