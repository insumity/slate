#!/bin/bash

execution_time=10
minimum_number_of_threads=12
increment=1
maximum_number_of_threads=12

loops_of_fp_operations=(0 1 2 4 128 256 512 1024)
loops_of_fp_operations=(0)

for threads in $(seq $minimum_number_of_threads $increment $maximum_number_of_threads)
do
    for loops in "${loops_of_fp_operations[@]}"
    do
	echo "memory" 
	echo ${memory_s}
	
	final_result=0
	policy=0
	sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/hyperthreads -t $threads -s $policy -l $loops -p 2>new_hyperthreads/loc_pinned_with_cores_threads_${threads}_loops_${loops} &

	sleep $execution_time;
	sudo pkill -INT hyperthreads;
	sudo pkill -9 gather_data;
	mv total_loops_* new_hyperthreads/


	policy=1
	sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/hyperthreads -t $threads -s $policy -l $loops -p 2>new_hyperthreads/loc_pinned_with_rr_threads_${threads}_loops_${loops} &

	sleep $execution_time;
	sudo pkill -INT hyperthreads;
	sudo pkill -9 gather_data;
	mv total_loops_* new_hyperthreads/


	policy=2 # HWCS policy 
	sudo ./gather_data $final_result $policy $threads /localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks/hyperthreads -t $threads -s $policy -l $loops -p 2>new_hyperthreads/loc_pinned_with_hwcs_threads_${threads}_loops_${loops} &

	sleep $execution_time;
	sudo pkill -INT hyperthreads;
	sudo pkill -9 gather_data;
	mv total_loops_* new_hyperthreads/

    done
done	   

