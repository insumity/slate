#!/usr/bin/env python3
import statistics
import os

papi = "gcc benchmark.c -o benchmark_papi -lpapi -lpthread -DPAPI_ENABLED -DPAPI_MULTIPLEX"
normal = "gcc benchmark.c -o benchmark_normal -lpapi -lpthread"

os.system(papi)
os.system(normal)

for th in [1, 2, 10, 20, 40]:
    TIMES = 2

    if th != 1 and th % 2 == 1:
        continue
    ar = []
    for i in range(0, TIMES):
        filename = str(th) + "papi"
        os.system("./benchmark_papi /localhome/kantonia/slate/papi_benchmark/program " + str(th) + " 2>>" + filename)
        f = open(filename, 'r')
        for line in f:
            print(" ... " + line)
            ar.append(float(line))
        f.close()

    avg = statistics.mean(ar)
    prev_avg = avg
    std = statistics.stdev(ar)
    print("{0:.0f}".format(th) + " :  " + "{0:.2f}".format(avg) + " " + "{0:.2f}".format(std))

    
    if th != 1 and th % 2 == 1:
        continue
    ar = []
    for i in range(0, TIMES):
        filename = str(th) + "normal"
        os.system("./benchmark_normal /localhome/kantonia/slate/papi_benchmark/program " + str(th) + " 2>>" + filename + " 1>>/dev/null")
        f = open(filename, 'r')
        for line in f:
            ar.append(float(line))
        f.close()

    avg = statistics.mean(ar)
    std = statistics.stdev(ar)
    print("{0:.0f}".format(th) + " :  " + "{0:.2f}".format(avg) + " " + "{0:.2f}".format(std))
    print(str(100.0 * (prev_avg / avg) - 100.0))

