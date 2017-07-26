#!/usr/bin/env python3
import csv
import sys
import glob
import math


filename = sys.argv[1]
f = open(filename, 'r')
reader = csv.reader(f, delimiter=' ')

min_per_col = 13 * [0]
max_per_col = 13 * [0]


# 0 1 51914536566 206299250 53185 200389704 28625 964811 20305 37247 0 10 5856361 0.000265337 1
# 0 1   2                                                            10 11 12       13         14

is_first_row = True
for row in reader:
    if is_first_row:
        for i in range(0, 13):
            min_per_col[i] = int(row[i])
            max_per_col[i] = int(row[i])
        is_first_row = False
        continue
    
    for i in range(0, 13):
        if (int(row[i]) < min_per_col[i]):
            min_per_col[i] = int(row[i])

        if (int(row[i]) > max_per_col[i]):
            max_per_col[i] = int(row[i])


print('{', end='')
for i in range(2, 13):
    print('{' + str(min_per_col[i] if min_per_col[i] >= 0 else 0) + ", " + str(max_per_col[i]) + '}, ', end='')
#print('{' + str(min_per_col[13]) + ', ' + str(max_per_col[13]) + '}' + '};')
print('};')

print('min_max = [', end='')
for i in range(2, 13):
    print('(' + str(min_per_col[i] if min_per_col[i] >= 0 else 0) + ", " + str(max_per_col[i]) + '), ', end='')
#print('{' + str(min_per_col[13]) + ', ' + str(max_per_col[13]) + '}' + '};')
print('];')



