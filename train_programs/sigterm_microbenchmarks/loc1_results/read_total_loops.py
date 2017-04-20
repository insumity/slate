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
    print(loc)
    print(rr)
    print("---")
