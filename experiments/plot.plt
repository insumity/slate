#!/usr/bin/gnuplot
clear
reset
unset key
# Make the x axis labels easier to read.
set xtics rotate out
# Select histogram data
set style data histogram
# Give the bars a plain fill pattern, and draw a solid line around them.
set style fill solid border
set style histogram clustered gap 1
set boxwidth 1 relative
set ylabel "Time (s)"
set xlabel "Applications"
set yrange [0:]
set terminal pngcairo
set output result_file
plot for [COL=from:to] filename using COL:xticlabels(1) title columnheader
