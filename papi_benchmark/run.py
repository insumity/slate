#!/usr/bin/env python3
import statistics
import os
import sys

#papi = "gcc benchmark.c -o benchmark_papi -lpapi -lpthread -DB_MANY -DB_PAPI_ENABLED -DB_READ_FOR=100 -DB_SLEEP_FOR=900 -lm -g"
papi = "gcc benchmark.c -o benchmark_papi -lpapi -lpthread -DB_MANY -DB_PAPI_ENABLED -DB_PAPI_MULTIPLEX -DB_READ_FOR=100 -DB_SLEEP_FOR=900 -lm -g"
normal = "gcc benchmark.c -o benchmark_normal -lpapi -lpthread -lm -g"

length = len(sys.argv) - 1
program = ""
for i in range(0, length - 1):
    program = program + " " + sys.argv[i + 1]

print("===" + program)
    
os.system(papi)
os.system(normal)

init_program = program
for th in [10, 20, 30]:
    TIMES = 3
    program = init_program + " " + str(th)
    print("<<<" + program)

    ar = []
    for i in range(0, TIMES):
        filename = str(th) + "papi"
        os.system("./benchmark_papi " + program + " 2> " + filename)
        f = open(filename, 'r')
        lines = 0
        for line in f:
            lines += 1
            if (lines <= 0):
                continue
            ar.append(float(line))
        f.close()

    avg = statistics.mean(ar)
    prev_avg = avg
    std = statistics.stdev(ar)
    print("{0:.0f}".format(th) + " :  " + "{0:.2f}".format(avg) + " " + "{0:.2f}".format(std))

    
    ar = []
    for i in range(0, TIMES):
        filename = str(th) + "normal"
        os.system("./benchmark_normal " + program + " 2> " + filename)
        f = open(filename, 'r')
        lines = 0
        for line in f:
            lines += 1
            if (lines <= 0):
                continue
            ar.append(float(line))
        f.close()

    avg = statistics.mean(ar)
    std = statistics.stdev(ar)
    print("{0:.0f}".format(th) + " :  " + "{0:.2f}".format(avg) + " " + "{0:.2f}".format(std))
    print("\n\n\n\t:" + str(100.0 * (prev_avg / avg) - 100.0) + "\n\n\n")

os.system("rm *papi *normal")

