#!/bin/bash

for i in $(seq 1 60)
do
    sudo perf stat -e r04d1:u -e r20d1:u -e r01d3:u -e r04d3:u -e r10d1:u -e r01C2:u -e r003c:u -e r20d3:u -e r10d3:u -C 0,4,8,12,16 2>perf.results$i & sleep 1; sudo pkill -INT perf

#sudo perf stat -e r04d1:u -e r20d1:u -e r03d3:u -e r0cd3:u -e r04cb:u -e r01C2:u -e r003c:u -e r20d3:u -e r10d3:u -C 0,4,8,12,16 2>perf.results$i & sleep 1; sudo pkill -INT perf
    echo "$i"
done

