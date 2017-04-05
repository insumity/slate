#!/usr/bin/env python3

import os
import timeit

# for i in range(2, 21):
#     os.system("gcc bandwidth2.c -o bandwidth2 -lpthread -lnuma -D LOC_CORES")
#     start = timeit.default_timer()
#     os.system("./bandwidth2 " + str(i) + " 1 1>/dev/null 2>/dev/null")
#     stop = timeit.default_timer()
#     timeLOC = stop - start

#     os.system("gcc bandwidth2.c -o bandwidth2 -lpthread -lnuma")
#     start = timeit.default_timer()
#     os.system("./bandwidth2 " + str(i) + " 1 1>/dev/null 2>/dev/null")
#     stop = timeit.default_timer()
#     timeRR = stop - start

#     os.system("gcc bandwidth2.c -o bandwidth2 -lpthread -lnuma")
#     start = timeit.default_timer()
#     os.system("./bandwidth2 " + str(i) + " 1>/dev/null 2>/dev/null")
#     stop = timeit.default_timer()
#     timeLinux = stop - start

#     print(str(i) + ":\t" + str(timeLOC) + "\t" +str(timeRR) + "\t" + str(timeLinux))


for i in range(2, 21):
    os.system("gcc locality_MIN_LAT_CORES.c -o locality -lpthread -lnuma -D LOC_CORES")
    start = timeit.default_timer()
    os.system("./locality " + str(i) + " 1 1>/dev/null 2>/dev/null")
    stop = timeit.default_timer()
    timeLOC = stop - start

    os.system("gcc locality_MIN_LAT_CORES.c -o locality -lpthread -lnuma")
    start = timeit.default_timer()
    os.system("./locality " + str(i) + " 1 1>/dev/null 2>/dev/null")
    stop = timeit.default_timer()
    timeRR = stop - start

    os.system("gcc locality_MIN_LAT_CORES.c -o locality -lpthread -lnuma")
    start = timeit.default_timer()
    os.system("./locality " + str(i) + " 1>/dev/null 2>/dev/null")
    stop = timeit.default_timer()
    timeLinux = stop - start

    print(str(i) + ":\t" + str(timeLOC) + "\t" +str(timeRR) + "\t" + str(timeLinux))


