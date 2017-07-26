#!/usr/bin/env python3

import sys

min_thread = int(sys.argv[1])
max_thread = int(sys.argv[2])

for i in range(min_thread, max_thread + 1):
    f = open("total_loops_threads_" + str(i) + "_policy_0_pin_1", "r")
    s = f.readline()
    loc = float(f.readline())

    f = open("total_loops_threads_" + str(i) + "_policy_1_pin_1", "r")
    s = f.readline()
    rr = float(f.readline())

    print("Threads " + str(i))
    if loc > rr:
        print(str(loc) + " ({0:.1f}%)".format(float(100 * (loc/rr - 1))))
        print(str(rr))
    else:
        print(str(loc))
        print(str(rr) + " ({0:.1f}%)".format(float(100 * (rr/loc - 1))))
        
    print("---")
