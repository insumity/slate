#!/bin/bash

DEMO_DIR="/localhome/kantonia/slate/train_programs/NEW_sigterm_microbenchmarks"

policies=( 0 1 )
for POLICY in "${policies[@]}"
do
    pkill -9 python

    (cd $DEMO_DIR && numactl --physcpubind=95 ./demo_poly.py testing2.csv $POLICY &)

    sleep 2 # wait 2 seconds to make sure the model got trained

    echo "about to run application"
#    files=( "ONE_WC_6" "ONE_WC_10" "ONE_WC_20" "ONE_MATRIXMULT_6" "ONE_MATRIXMULT_10" "ONE_MATRIXMULT_20" "ONE_WR_6" "ONE_WR_10" "ONE_WR_20" "ONE_WRMEM_6" "ONE_WRMEM_10" "ONE_WRMEM_20" "ONE_BLACKSCHOLES_6" "ONE_BLACKSCHOLES_10" "ONE_BLACKSCHOLES_20" "ONE_BODYTRACK_6" "ONE_BODYTRACK_10" "ONE_BODYTRACK_20" "ONE_CANNEAL_6" "ONE_CANNEAL_10" "ONE_CANNEAL_20" "ONE_FACESIM_4" "ONE_FACESIM_8" "ONE_FACESIM_16" "ONE_FERRET_6" "ONE_FERRET_10" "ONE_FERRET_22" "ONE_FLUIDANIMATE_4" "ONE_FLUIDANIMATE_8" "ONE_FLUIDANIMATE_16" "ONE_RAYTRACE_6" "ONE_RAYTRACE_10" "ONE_RAYTRACE_20" "ONE_SC_6" "ONE_SC_10" "ONE_SC_20" "ONE_SW_6" "ONE_SW_10" "ONE_SW_20" "ONE_VIPS_6" "ONE_VIPS_10" "ONE_VIPS_20" )

    # the followins one were executed latest
    #    files=( "ONE_PCA_6" "ONE_PCA_10" "ONE_PCA_20" "ONE_HIST_6" "ONE_HIST_10" "ONE_HIST_20" "ONE_KMEANS_6" "ONE_KMEANS_10" "ONE_KMEANS_20" "ONE_STRING_MATCH_6" "ONE_STRING_MATCH_10" "ONE_STRING_MATCH_20" "ONE_LINEAR_REGRESSION_6" "ONE_LINEAR_REGRESSION_10" "ONE_LINEAR_REGRESSION_20" )

    #files=( "ONE_KYOTOCABINET_6" "ONE_KYOTOCABINET_10" "ONE_KYOTOCABINET_20" )
    files=( "ONE_KYOTOCABINET_6" "ONE_UPSCALEDB_6" "ONE_UPSCALEDB_10" "ONE_UPSCALEDB_20" )

    iterations=( 1 2 3 )

    for APP_FILE_NAME in "${files[@]}"
    do
	for ITERATION in "${iterations[@]}"
	do
	    sudo ./schedule $APP_FILE_NAME foo. GREEDY true $POLICY 2>gathered_counter_results/$APP_FILE_NAME.pol_$POLICY.$ITERATION.results
	done
    done

    pkill -9 python
done
