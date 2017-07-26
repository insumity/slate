#!/usr/bin/env python3

import sys

input_filename = sys.argv[1]

input_f = open(input_filename, 'r')

#min_max = [(124264251, 109110801742), (246097, 47882468310), (0, 484779018), (35, 1129030691), (0, 139257884), (0, 29103353), (0, 8706227), (0, 27922638), (0, 497), (2, 23)]

#min_max = [(0, 77436373389), (0, 304460564), (0, 83136354), (0, 292501258), (0, 84155769), (0, 20063832), (0, 5437316), (0, 17101458), (0, 374), (2, 14), (0, 89967054)];

min_max = [(0, 77436373389), (0, 304460564), (0, 83136354), (0, 292501258), (0, 84155769), (0, 20063832), (0, 5437316), (0, 17101458), (0, 374), (2, 14), (0, 89967054)];


line1 = input_f.readline()
while line1:
    line2 = input_f.readline()
    if not line2:
        break

    line1_s = line1.split(" ")
    r = [0] * 10

    print(str(int(line1_s[0])) + ", " + str(int(line1_s[1])) + ", ", end='')
    for i in range(0, 10):
        nom = int(line1_s[i + 2]) - min_max[i][0]
        den = float(min_max[i][1] - min_max[i][0])
        r[i] = nom / den
        print (str(r[i]) + ", ", end='')


    #print(str(load_perc) + ", " + str(LLC_miss_rate) + ", " + str(remote), end='')
    print(str(float(line1_s[13])) + ", " + str(float(line1_s[14])))

        
    line2_s = line2.split(" ")
    r = [0] * 10

    print(str(int(line2_s[0])) + ", " + str(int(line2_s[1])) + ", ", end='')
    for i in range(0, 10):
        nom = int(line2_s[i + 2]) - min_max[i][0]
        den = float(min_max[i][1] - min_max[i][0])
        r[i] = nom / den
        print (str(r[i]) + ", ", end='')
    print(str(float(line1_s[13])) + ", " + str(float(line1_s[14])))


    line1 = input_f.readline()

input_f.close()


