#!/bin/bash

execution_time=25
minimum_number_of_threads=1
maximum_number_of_threads=48

for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
do
    final_result=0
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/new_microbenchmarks/loc_perf -t $threads -s $policy -e $execution_time -p 2>loc1_results/loc_pinned_with_loc_threads$threads

    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/new_microbenchmarks/loc_perf -t $threads -s $policy -e $execution_time -p 2>loc1_results/loc_pinned_with_rr_threads$threads

    mv total_loops_* loc1_results/
done	   

for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
do
    final_result=0
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/new_microbenchmarks/loc2_perf -t $threads -s $policy -e $execution_time -p 2>loc2_results/loc_pinned_with_loc_threads$threads

    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/new_microbenchmarks/loc2_perf -t $threads -s $policy -e $execution_time -p 2>loc2_results/loc_pinned_with_rr_threads$threads

    mv total_loops_* loc2_results/
done	   

for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
do
    final_result=0
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/new_microbenchmarks/loc3_perf -t $threads -s $policy -e $execution_time -p 2>loc3_results/loc_pinned_with_loc_threads$threads

    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/new_microbenchmarks/loc3_perf -t $threads -s $policy -e $execution_time -p 2>loc3_results/loc_pinned_with_rr_threads$threads

    mv total_loops_* loc3_results/
done

for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
do
    final_result=1
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/new_microbenchmarks/rr_perf -t $threads -s $policy -e $execution_time -p 2>rr1_results/rr_pinned_with_loc_threads$threads

    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/new_microbenchmarks/rr_perf -t $threads -s $policy -e $execution_time -p 2>rr1_results/rr_pinned_with_rr_threads$threads

    mv total_loops_* rr1_results/
done	   

for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
do
    final_result=1
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/new_microbenchmarks/rr2_perf -t $threads -s $policy -e $execution_time -p 2>rr2_results/rr_pinned_with_loc_threads$threads

    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/new_microbenchmarks/rr2_perf -t $threads -s $policy -e $execution_time -p 2>rr2_results/rr_pinned_with_rr_threads$threads

    mv total_loops_* rr2_results/
done	   

