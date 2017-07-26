#!/usr/bin/env python3
import sys
import csv
import string

def f(LLC, local, A, B):
    return LLC * local * (float(A)) + LLC * (1.0 - local) * B

def get_file_data(file_name, column1, column2, column3, column4, A, B):
    res = True
    cnt = 0
    result = []
    with open(file_name, 'r') as csvfile:
        reader = csv.reader(csvfile, delimiter='\t')
        for row in reader:
            if res:
                res = False
                continue
            res1 = f(float(row[column1]), float(row[column2]), A, B)
            res2 = f(float(row[column3]), float(row[column4]), A, B)

            t = (cnt, res1, res2)
            result.append(t)
            cnt = cnt + 1

    sorted_res = sorted(result, key=lambda tup: tup[1])
    cnt = 0
    for i in sorted_res:
        #       if (cnt == 0 and i[0] != 8):
        #            print("EAT SHIT\n")
            
        print(str(i[0]) + "\t\t" + str(i[1]) + "\t\t" + str(i[2]))
        cnt = cnt + 1


for A in [1.5, 2, 2.1, 2.2, 2.3, 2.4, 2.5]:
    for B in [2, 2.1, 2.2, 2.3, 2.4, 2.5, 3, 3.5, 4, 4.1, 4.2, 4.3, 4.4, 4.4, 4.5, 5]:
        print(str(A) + ", " + str(B))
        get_file_data(sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5]), A, B)
        print("===\n")

print("====\n")
get_file_data(sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5]), float(sys.argv[6]), float(sys.argv[7]))
