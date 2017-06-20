#!/bin/bash

execution_time=6
minimum_number_of_threads=2
increment=4
maximum_number_of_threads=22

memory_size=(1 2 4 256 1024) # 262144 maybe for higher size??
readers_perc=(0 50 100)
nodes=(0 1 2 3)

for threads in $(seq $minimum_number_of_threads $increment $maximum_number_of_threads)
do
    for memory_s in "${memory_size[@]}"
    do
	for readers_p in "${readers_perc[@]}"
	do
	    for node in "${nodes[@]}"
	    do

		echo "memory" 
		echo ${memory_s}
    
		final_result=0
		policy=0
		sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/new_loc -t $threads -s $policy -z $memory_s -r $readers_p -n $node -p 2>new_loc_results/loc_pinned_with_loc_memory_${memory_s}threads_${threads}_readers_${readers_p}_node_${node} &

		sleep $execution_time;
		sudo pkill -INT new_loc;
		sudo pkill -9 gather_data;
		mv total_loops_* new_loc_results/


		policy=1
		sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/new_loc -t $threads -s $policy -z $memory_s -r $readers_p -n $node -p 2>new_loc_results/loc_pinned_with_rr_memory_${memory_s}threads_${threads}_readers_${readers_p}_node_${node} &

		sleep $execution_time;
		sudo pkill -INT new_loc;
		sudo pkill -9 gather_data;
		mv total_loops_* new_loc_results/

	    done
	done
    done
done	   

