#!/usr/bin/env python3

import sys

f = open(sys.argv[1], "r")

def get_numbers(line):
    return line.split("\t\t")

prev_numbers = get_numbers(f.readline())
while prev_numbers:
    s = f.readline()
    if not s:
        break
    current_numbers = get_numbers(s)
    if not current_numbers:
        break

    for i in range(1, len(current_numbers)):
        n1 = int(prev_numbers[i])
        n2 = int(current_numbers[i])
        print(str(n1) + " ::: " + str(n2) + " ::: " + str(n2 >= n1) + " ::: " + str(n2 - n1))

    prev_numbers = current_numbers[:]

f.close()
