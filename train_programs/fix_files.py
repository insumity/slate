#!/usr/bin/env python3

import sys

input_filename = sys.argv[1]

input_f = open(input_filename, 'r')



#{{0, 1}, {0, 1}, {1092759414, 33925413580}, {176229937, 13023200198}, {3145818, 339093184}, {7320366, 459744183}, {593462, 38538182}, {188286, 22675395}, {1098, 3748001}, {48, 14774767}, {13655, 850725}, {20, 21}, {0, 1}};
min_max = [(1092759414, 33925413580), (176229937, 13023200198), (3145818, 339093184), (7320366, 459744183), (593462, 38538182), (188286, 22675395), (1098, 3748001), (48, 14774767), (13655, 850725), (20, 21)]


line1 = input_f.readline()
while line1:
    line2 = input_f.readline()

    line1_s = line1.split(",")
    r = [0] * 10

    print(str(int(line1_s[0])) + ", " + str(int(line1_s[1])) + ", ", end='')
    for i in range(0, 10):
        nom = int(line1_s[i + 2]) - min_max[i][0]
        den = float(min_max[i][1] - min_max[i][0])
        r[i] = nom / den
        print (str(r[i]) + ", ", end='')

    print(line1_s[12], end='')
        
    line2_s = line2.split(",")
    r = [0] * 10

    print(str(int(line2_s[0])) + ", " + str(int(line2_s[1])) + ", ", end='')
    for i in range(0, 10):
        nom = int(line2_s[i + 2]) - min_max[i][0]
        den = float(min_max[i][1] - min_max[i][0])
        r[i] = nom / den
        print (str(r[i]) + ", ", end='')

    print(line2_s[12], end='')

    line1 = input_f.readline()

input_f.close()


