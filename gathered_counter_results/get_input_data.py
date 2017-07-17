#!/usr/bin/env python3

import sys, csv, glob, operator

SIZE_OF_LINE_WITH_DATA = 19

# returns previous all lines that contain perf. counters except the lines that appear after "error" msgs start appear on screen
# so if the file looks like this:
# counters1
# garbage 
# counters2
# ...
# countersN - 1
# countersN
# garbage (couldn't find file msg)
# counterN + 1
# counterN + 2
# will return lines 1 to 19
def read_file(filename):
    input_lines_started = False
    last_line = "NONE"
    previous_to_last_line = "NONE"

    all_rows = []
    len_rows = []
    with open(filename, 'r') as csvfile:
        spamreader = csv.reader(csvfile, delimiter=',', quotechar='|')
        for row in spamreader:
            all_rows.append(row)
            len_rows.append(len(row))


    resulted_lines = []
    started = False
    for i in range(0, len(all_rows)):
        if len_rows[i] == SIZE_OF_LINE_WITH_DATA:
            started = True
            resulted_lines.append(all_rows[i])
            print(print_list(all_rows[i]))
        if started and len_rows[i] != SIZE_OF_LINE_WITH_DATA:
            break

def print_list(l):
    result = ""
    for elem_index in range(0, len(l) - 1):
        result = result + l[elem_index] + ","
    result = result + l[len(l) - 1]
    return result

for directory_index in range(1, len(sys.argv)):
    directory = sys.argv[directory_index]
    tmp = glob.glob(directory)
    for filename in tmp:
        read_file(filename)

