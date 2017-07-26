#!/usr/bin/env python3

import sys, subprocess

min_thread = int(sys.argv[1])
max_thread = int(sys.argv[2])


def execute(cmd):
    popen = subprocess.Popen(cmd, stdout=subprocess.PIPE, universal_newlines=True)
    for stdout_line in iter(popen.stdout.readline, ""):
        yield stdout_line
    popen.stdout.close()
    return_code = popen.wait()
    if return_code:
        raise subprocess.CalledProcessError(return_code, cmd)

def get_one_data(policy, threads, loops):
    pol_str = "rr"
    if policy == 0:
        pol_str = "cores"
    elif policy == 2:
        pol_str = "hwcs"

    l = ""
    for o in execute(["./get_input_data.py",  str("loc_pinned_with_") + str(pol_str) + str("_threads_") + str(threads) + "_loops_" + str(loops)]):
        l = o

    pl = l.split(",")
    return pl

    
def get_data(policy, i, loops):

    pol_str = "rr"
    if policy == 0:
        pol_str = "cores"
    elif policy == 2:
        pol_str = "hwcs"
    
    l = ""
    for o in execute(["./get_input_data.py",  str("loc_pinned_with_") + str(pol_str) + str("_threads_") + str(i) + "_loops_" + str(loops)]):
        l = o

    pl = l.split(",")
#    print(pl, end="")
    return str(int(pl[5]) / float(pl[8]))
    

for loops in [0, 1, 2, 4, 8, 16, 32, 64]:
#    print("\t\t\n---\nLoops: " + str(loops) + "\n")
    for i in range(min_thread, max_thread + 1):


        try:
            f = open("total_loops_threads_" + str(i) + "_policy_0_pin_1_loops_" + str(loops), "r")
        except FileNotFoundError:
            continue
    
        s = f.readline()
        loc = float(f.readline())

        try:
            f = open("total_loops_threads_" + str(i) + "_policy_1_pin_1_loops_" + str(loops), "r")
        except FileNotFoundError:
            continue
        
        s = f.readline()
        rr = float(f.readline())


        try:
            f = open("total_loops_threads_" + str(i) + "_policy_2_pin_1_loops_" + str(loops), "r")
        except FileNotFoundError:
            continue

        s = f.readline()
        hwcs = float(f.readline())

        
#        print("Threads " + str(i))
 #       print("(hwcs: " + str(hwcs) + " " + get_data(2, i, loops) +") : (loc: " + str(loc) + " " + get_data(0, i, loops) + ") : (rr: " + str(rr) + " " + get_data(1, i, loops) +  ")")



        final_result = 0
        if hwcs > loc and hwcs > 1.1 * loc: # hwcs is better
            final_result = " 2"
        elif loc > hwcs and loc > 1.1 * hwcs:
            final_result = " 0"
        else:
            continue

            
        res = get_one_data(0, i, loops)
        res[len(res) - 1] = final_result
        print(",".join(res))
        res = get_one_data(2, i, loops)
        res[len(res) - 1] = final_result
        print(",".join(res))


        # if loc > rr:
        #     print(str(loc) + " ({0:.1f}%)".format(float(100 * (loc/rr - 1))))
        #     print(str(rr))
        # else:
        #     print(str(loc))
        #     print(str(rr) + " ({0:.1f}%)".format(float(100 * (rr/loc - 1))))
        
#        print("---")
