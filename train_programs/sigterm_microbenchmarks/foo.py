#!/usr/bin/env python3

import sys
import csv

fname = sys.argv[1]
sfname = sys.argv[2]

elements = 0
summy = [0.] * 17
with open(fname, 'r') as csvfile:
    spamreader = csv.reader(csvfile, delimiter=',', quotechar='|')
    for row in spamreader:
        for i in range(0, 17):
            summy[i] = summy[i] + float(row[i])
        elements = elements + 1


for i in range(0, 17):
    summy[i] = summy[i] / elements


results = [0.] * 17
with open(fname, 'r') as csvfile:
    spamreader = csv.reader(csvfile, delimiter=',', quotechar='|')
    for row in spamreader:
        for i in range(0, 17):
            results[i] = (float(row[i]) - summy[i]) ** 2.


import math

for i in range(0, 17):
    results[i] = math.sqrt(results[i] / elements)

print(summy)
print(results)


with open(sfname, 'r') as csvfile:
    spamreader = csv.reader(csvfile, delimiter=',', quotechar='|')
    for row in spamreader:
        for i in range(2, 17):
            n = float(row[i])
            miny = summy[i] - results[i]
            maxy = summy[i] + results[i]
            if n < miny or n > maxy:
                print(row)
                print("for i: " + str(i) + " === " + str(n) + " min(" + str(miny) + ", " + str(maxy) + ")");
                print("-----")
                break

