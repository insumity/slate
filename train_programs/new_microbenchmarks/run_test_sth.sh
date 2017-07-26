#!/bin/bash



loops=(3 4 5 6 7 8 9)
for loop in "${loops[@]}"
do
    sudo ./gather_data 0 2 /localhome/kantonia/slate/train_programs/microbenchmarks/TESTING_STH -t 2 -s 0 -l $loop -p 2>test_sth.results_loc$loop &
    sleep 23;
    sudo pkill -9 gather_d;
    sudo pkill -9 TESTING_STH;

    sudo ./gather_data 1 2 /localhome/kantonia/slate/train_programs/microbenchmarks/TESTING_STH -t 2 -s 1 -l $loop -p 2>test_sth.results_rr$loop &
    sleep 23;
    sudo pkill -9 gather_data;
    sudo pkill -9 TESTING_STH;
done	   
