#!/usr/bin/env python3

import sys
import os

min_thread = int(sys.argv[1])
max_thread = int(sys.argv[2])

for i in range(min_thread, max_thread + 1):
    for memory_size in [1, 2, 4, 256, 1024]:
        for readers_p in [0, 50, 100]:
            for node in [0, 1, 2, 3]:
                try:

                    f = open("total_loops_threads_" + str(i) + "_policy_0_pin_1" + "_memory_size_" +
                         str(memory_size) + "_readers_percentage_" + str(readers_p) + "_memory_node_" +
                         str(node), "r")
                except FileNotFoundError:
                    continue
            
                s = f.readline()
                loc = float(f.readline())


                f = open("total_loops_threads_" + str(i) + "_policy_1_pin_1" + "_memory_size_" +
                     str(memory_size) + "_readers_percentage_" + str(readers_p) + "_memory_node_" +
                     str(node), "r")

                s = f.readline()
                rr = float(f.readline())

                if loc > rr and 100 * (loc / rr - 1) > 100:

                    filename = "loc_pinned_with_loc_memory_" + str(memory_size) + "threads_" + str(i) + "_readers_" + str(readers_p) + "_node_" + str(node)
                    os.system("cat " + filename + " >> /tmp/foobarisios007")
                    
                    print("Threads " + str(i) + " memroy size: " + str(memory_size) + " readers_p " + str(readers_p) + " node: " + str(node))
                    
                    print(str(loc) + " ({0:.1f}%)".format(float(100 * (loc/rr - 1)))) 
                    print(str(rr))
                    print("---")

                else:
                    if 100 * (rr / loc - 1) > 100:
                        filename = "loc_pinned_with_rr_memory_" + str(memory_size) +"threads_" + str(i) + "_readers_" + str(readers_p) + "_node_" + str(node)
                        os.system("cat " + filename + " >> /tmp/foobarisios007")


                        print("Threads " + str(i) + " memroy size: " + str(memory_size) + " readers_p " + str(readers_p) + " node: " + str(node))

                        print(str(loc))
                        print(str(rr) + " ({0:.1f}%)".format(float(100 * (rr/loc - 1))))
        
                        print("---")
