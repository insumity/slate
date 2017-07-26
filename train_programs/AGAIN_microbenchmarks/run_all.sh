#!/bin/bash

execution_time=20
minimum_number_of_threads=19
maximum_number_of_threads=20

# for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
# do
#     final_result=0
#     policy=0
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/AGAIN_microbenchmarks/loc -t $threads -s $policy -p 2>loc1_results/loc_pinned_with_loc_threads$threads &

#     sleep $execution_time;
#     sudo pkill -9 gather_data;
#     sudo pkill -9 loc;


#     policy=1
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/AGAIN_microbenchmarks/loc -t $threads -s $policy -p 2>loc1_results/loc_pinned_with_rr_threads$threads &

#     sleep $execution_time;
#     sudo pkill -9 gather_data;
#     sudo pkill -9 loc;
# done	   

# for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
# do
#     final_result=0
#     policy=0
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/AGAIN_microbenchmarks/loc2 -t $threads -s $policy -p 2>loc2_results/loc_pinned_with_loc_threads$threads &

#     sleep $execution_time;
#     sudo pkill -9 gather_data;
#     sudo pkill -9 loc2;

#     policy=1
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/AGAIN_microbenchmarks/loc2 -t $threads -s $policy -p 2>loc2_results/loc_pinned_with_rr_threads$threads &


#     sleep $execution_time;
#     sudo pkill -9 gather_data;
#     sudo pkill -9 loc2;
# done	   

# for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
# do
#     final_result=0
#     policy=0
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/AGAIN_microbenchmarks/loc3 -t $threads -s $policy -p 2>loc3_results/loc_pinned_with_loc_threads$threads &

#     sleep $execution_time;
#     sudo pkill -9 gather_data;
#     sudo pkill -9 loc3;


#     policy=1
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/AGAIN_microbenchmarks/loc3 -t $threads -s $policy -p 2>loc3_results/loc_pinned_with_rr_threads$threads &

#     sleep $execution_time;
#     sudo pkill -9 gather_data;
#     sudo pkill -9 loc3;

# done

# for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
# do
#     final_result=1
#     policy=0
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/AGAIN_microbenchmarks/rr -t $threads -s $policy -p 2>rr1_results/rr_pinned_with_loc_threads$threads &

#     sleep $execution_time;
#     sudo pkill -9 gather_data;
#     sudo pkill -9 rr;

#     policy=1
#     sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/AGAIN_microbenchmarks/rr -t $threads -s $policy -p 2>rr1_results/rr_pinned_with_rr_threads$threads &

#     sleep $execution_time;
#     sudo pkill -9 gather_data;
#     sudo pkill -9 rr;
# done	   

for threads in $(seq $minimum_number_of_threads $maximum_number_of_threads)
do
    final_result=1
    policy=0
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/AGAIN_microbenchmarks/rr2 -t $threads -s $policy -p 2>rr2_results/rr_pinned_with_loc_threads$threads &

    
    sleep $execution_time;
    sudo pkill -9 gather_data;
    sudo pkill -9 rr2;

    policy=1
    sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/AGAIN_microbenchmarks/rr2 -t $threads -s $policy -p 2>rr2_results/rr_pinned_with_rr_threads$threads &

    sleep $execution_time;
    sudo pkill -9 gather_data;
    sudo pkill -9 rr2;

done	   

