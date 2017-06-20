#!/usr/bin/env python3

import sys, csv, glob, operator

SIZE_OF_LINE_WITH_DATA = 19

# returns previous to last line of the generated file that contains the perf. counters
# so if the file looks like this:
# counters1
# garbage 
# counters2
# ...
# countersN - 1
# countersN
# garbage (couldn't find file msg)
# counterN + 1
# it will return the line of "countersN - 1" (actually will try to find the longest subsequence of contiguous input counters and return the previous
# to the last one)
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


    # find longest subseqeunce inside len_rows that contains 19's
    nineteens_rows = len(len_rows) * [0]
    for i in range(len(len_rows) -1, - 1, -1):
        if i == len(len_rows) - 1:
            nineteens_rows[i] = 0
        else:
            nineteens_rows[i] = 0 if len_rows[i] != 19 else 1 + nineteens_rows[i + 1]


    index, value = max(enumerate(nineteens_rows), key=operator.itemgetter(1))
    #print(len_rows)
    #    print(nineteens_rows)
    #   print((index, value))

    #  print(len(len_rows))

    if value > 1:
        previous_to_last_index = index + (value - 2)

        return (all_rows[previous_to_last_index], all_rows[previous_to_last_index - 1], all_rows[previous_to_last_index -2],
                all_rows[previous_to_last_index - 3], all_rows[previous_to_last_index - 4], all_rows[previous_to_last_index - 5],
                all_rows[previous_to_last_index - 6])
    else:
        return (all_rows[index], )

#print(read_file(sys.argv[1]))


def print_list(l):

    for k in range(0, len(l)):
        string = l[k]
        result = ""
        for elem_index in range(0, len(string) - 1):
            result = result + string[elem_index] + ", "
        result = result + string[len(string) - 1]
        print(result)

for directory_index in range(1, len(sys.argv)):
    directory = sys.argv[directory_index]
    loc_list = glob.glob(directory + "/loc*")
    for filename in loc_list:
        print_list(read_file(filename))
    rr_list = glob.glob(directory + "/rr*")
    for filename in rr_list:
        print_list(read_file(filename))


