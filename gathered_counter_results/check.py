#!/usr/bin/env python2.7

from __future__ import print_function
import numpy as np
import sys, glob
from tabulate import tabulate
from sklearn import linear_model, svm, tree, neighbors
from sklearn.neighbors.nearest_centroid import NearestCentroid
from sklearn.linear_model import *
from sklearn.ensemble import *
from sklearn.dummy import *
from sklearn.neural_network import MLPClassifier
from sklearn.preprocessing import *
from sklearn.gaussian_process import GaussianProcessClassifier
from sklearn.naive_bayes import GaussianNB
from sklearn.discriminant_analysis import QuadraticDiscriminantAnalysis
from sklearn.decomposition import PCA
from sklearn.pipeline import Pipeline
from sklearn import preprocessing
from sklearn.model_selection import cross_val_score
import os

#print_all = True
print_all = False

printed_already = {}
printed_already_rr = {}

polynomial = False
polynomial_number = 3

def to_new_sample(data, printy):
    LOC = data[0]
    RR = data[1]
    l3_hit = data[2]
    l3_miss = data[3]
    local_dram = data[4]
    remote_dram = data[5]
    l2_miss = data[6]
    uops_retired = data[7]
    unhalted_cycles = data[8]
    remote_fwd = data[9]
    remote_hitm = data[10]
    instructions = data[11]
    context_switches = data[12]
    bw1 = data[13]
    bw2 = data[14]
    bw3 = data[15]
    bw4 = data[16]
    number_of_threads = data[17]

    #data = data.reshape(1, -1)
    # FIXME
    #return data
    f =  np.hstack((data[0:2], data[6:9], data[10:12], data[17]))
    return f

    intra_socket = l2_miss - (l3_hit + l3_miss)
    inter_socket = remote_fwd + remote_hitm
    bw = bw1 + bw2 + bw3 + bw4
    local_accesses = 0
    if local_dram > 0 or remote_dram > 0:
        local_accesses = local_dram / (1 + local_dram + float(remote_dram))

    normalized = 18 * [0.]
    normalized[0] = LOC
    normalized[1] = RR
    for i in range(2, 12):
        normalized[i] = (float(data[i]) / float(2 * 1000 * 1000 * 1000)) / float(number_of_threads)

    for i in range(12, 16):
        normalized[i] = float(data[i]) / float(number_of_threads)

    normalized[17] = data[17]

    llc_hitrate = l3_hit / float(l3_hit + l3_miss)
    new_data = np.array([LOC, RR, l3_miss, l3_hit, l2_miss, remote_hitm, unhalted_cycles, instructions, uops_retired, number_of_threads])


    new_data = np.array([LOC, RR, inter_socket / float(uops_retired), intra_socket / float(uops_retired), 
                         l3_hit / float(uops_retired), l3_miss / float(uops_retired),
                         l3_hit / (l3_hit + float(l3_miss)), float(uops_retired) / float(unhalted_cycles), bw, number_of_threads])

    D = float(uops_retired * number_of_threads)
     
    if printy:
        if LOC == 1:
            if number_of_threads in printed_already:
                if printed_already[number_of_threads] == 2 and not print_all:
                    return new_data
                elif printed_already[number_of_threads] == 1:
                    printed_already[number_of_threads] = printed_already[number_of_threads] + 1
            else:
                printed_already[number_of_threads] = 1

            print("LOC")
        else:
            if number_of_threads in printed_already_rr:
                if printed_already_rr[number_of_threads] == 2 and not print_all:
                    return new_data
                elif printed_already_rr[number_of_threads] == 1:
                    printed_already_rr[number_of_threads] = printed_already_rr[number_of_threads] + 1
            else:
                printed_already_rr[number_of_threads] = 1

            print("RR")
            
        print("number of threads: " + str(number_of_threads))
        print("llc hit rate: " + str(llc_hitrate))
        print("intra socket: " + str(intra_socket / float(uops_retired)))
        print("inter socket: " + str(inter_socket / float(uops_retired)))
        print("uops retired per cycle: " + str(uops_retired / float(unhalted_cycles)))
        print("llcc miss per uops retired: " + str(l3_miss / float(uops_retired)))
        print("bw1, bw2, bw3, bw4: " + str(bw1) + " " + str(bw2) + " " + str(bw3) + " " + str(bw4))
        print("(local + remote dram): " + str((local_dram + remote_dram) / float(uops_retired)))


        for k in new_data:
            print(str(k)  + ", ", end='')
            
        print()
        #print(data)
        print("------")

    #return new_data
    return data


def transform(all_data, printy):

    new_all_data = to_new_sample(all_data[0], printy)

    for i in range(1, all_data.shape[0]):
        data = all_data[i]
        new_data = to_new_sample(data, printy)
        new_all_data = np.vstack((new_all_data, new_data))

    return new_all_data




# initialize so we have them in classify
actual_train_data = []
labels = []


train_data = np.loadtxt(sys.argv[1], delimiter=',')
(rows, cols) = train_data.shape
labels = train_data[:, cols - 1]



actual_train_data = train_data[:, 0:cols - 1]
print("actual.shape: " + str(actual_train_data.shape))

pipeline = Pipeline([('scaling', StandardScaler()), ('pca', PCA(n_components=3))])

actual_train_data = transform(train_data[:, 0:cols - 1], False)

if polynomial:
    poly = PolynomialFeatures(polynomial_number)
    actual_train_data = poly.fit_transform(actual_train_data)


print("----- About to have a look at test_data -----")


classifiers = [QuadraticDiscriminantAnalysis(), LogisticRegression(random_state=42), BaggingClassifier(random_state=42), AdaBoostClassifier(random_state=42),
               PassiveAggressiveClassifier(random_state=42), DummyClassifier(random_state=42), RidgeClassifier(random_state=42), MLPClassifier(random_state=42),
               tree.DecisionTreeClassifier(random_state=42), NearestCentroid(), neighbors.KNeighborsClassifier(), SGDClassifier(random_state=42),
               RandomForestClassifier(random_state=42)]

classifier_names = ["Application", "#threads", "(executed with) Policy", "Time (s) loc", "Time (s) mem", "loc vs mem?", 
                    "QuadraticDiscriminantAnalysis", "LogisticRegressin", "Bagging", "AdaBoost", "PassiveAggressive", "Dummy", "Ridge", "MLP", "DecisionTree",
                    "NearestCentroid", "KNeighbors", "SGD", "RandomForest"]


wc_times = {10: {'mem': 12.8, 'loc': 8.32}, 20: {'mem': 19, 'loc': 15.7}, 6: {'mem': 10.6, 'loc': 8.36}}
matrixmult_times = {10: {'mem': 5.04, 'loc': 5.02}, 20: {'mem': 2.99, 'loc': 2.85}, 6: {'mem': 8, 'loc': 8.15}}
wr_times = {10: {'mem': 17.6, 'loc': 11.19}, 20: {'mem': 19.5, 'loc': 16.2}, 6: {'mem': 17, 'loc': 11.02}}
wrmem_times = {10: {'mem': 16.7, 'loc': 15.7}, 20: {'mem': 13.1, 'loc': 12.4}, 6: {'mem': 22.3, 'loc': 22.14}}

blackscholes_times = {10: {'mem': 3.85, 'loc': 3.88}, 20: {'mem': 3.2, 'loc': 3.21}, 6: {'mem': 4.83, 'loc': 5.01}}
bodytrack_times = {10: {'mem': 36.89, 'loc': 34.36}, 20: {'mem': 24.96, 'loc': 24.74}, 6: {'mem': 48, 'loc': 47}}
canneal_times = {10: {'mem': 27.0, 'loc': 18.8}, 20: {'mem': 15.45, 'loc': 13.85}, 6: {'mem': 41.85, 'loc': 28.4}}
facesim_times = {8: {'mem': 22.3, 'loc': 19.12}, 16: {'mem': 18.5, 'loc': 18.3}, 4: {'mem': 30.3, 'loc': 29.08}}
ferret_times = {10: {'mem': 36, 'loc': 38.5}, 22: {'mem': 15.6, 'loc': 16.1}, 6: {'mem': 70, 'loc': 73}}
fluidanimate_times = {8: {'mem': 21.2, 'loc': 22.1}, 16: {'mem': 13.5, 'loc': 13.4}, 4: {'mem': 35, 'loc': 38.26}}
raytrace_times = {10: {'mem': 80, 'loc': 79.7}, 20: {'mem': 70.5, 'loc': 69}, 6: {'mem': 91, 'loc': 92}}
streamcluster_times = {10: {'mem': 18.6, 'loc': 15.7}, 20: {'mem': 11, 'loc': 14}, 6: {'mem': 33, 'loc': 24.7}}
swaptions_times = {10: {'mem': 7.2, 'loc': 7.5}, 20: {'mem': 4.4, 'loc': 4.5}, 6: {'mem': 10.8, 'loc': 11.68}}
vips_times = {10: {'mem': 12.7, 'loc': 12.8}, 20: {'mem': 7.4, 'loc': 7.5}, 6: {'mem': 19, 'loc': 20}}

execution_times = {"WC": wc_times, "MATRIXMULT": matrixmult_times, "WR": wr_times, "WRMEM": wrmem_times, "BLACKSCHOLES": blackscholes_times,
                   "BODYTRACK": bodytrack_times, "CANNEAL": canneal_times, "FACESIM": facesim_times, "FERRET": ferret_times,
                   "FLUIDANIMATE": fluidanimate_times, "RAYTRACE": raytrace_times, "SC": streamcluster_times,
                   "SW": swaptions_times, "VIPS": vips_times}

test_filenames = glob.glob("*.results")
test_filenames.sort()



def get_actual_name(name):
    # extracts WRMEM from ONE_WRMEM_10.pol_1.2.results
    lst = name.split("_")
    return lst[1]

def get_threads(name):
    lst = name.split("_")
    return lst[2].split('.')[0]

def get_policy(name):
    lst = name.split("_")
    if int(lst[3].split('.')[0]) == 0:
        return "loc"
    else:
        return "mem"

it = iter(test_filenames)
to_print = []
for fn in it:
    fn1, fn2, fn3 = (fn, next(it), next(it))
    print("3 files: " + fn1 + ", " + fn2 + ", " + fn3)
    data = []
    app_name = get_actual_name(fn1)
    threads_number = get_threads(fn1)
    policy = get_policy(fn1)
    data.append(app_name)
    data.append(threads_number)
    data.append(policy)

    data.append(execution_times[app_name][int(threads_number)]["loc"])
    data.append(execution_times[app_name][int(threads_number)]["mem"])

    if (float(execution_times[app_name][int(threads_number)]["loc"]) < float(execution_times[app_name][int(threads_number)]["mem"])):
        data.append("loc")
    else:
        data.append("mem")

                    
    # get the average of 3 files
    avg = len(classifiers) * [0.0]

    for fil in [fn1, fn2, fn3]:
        os.system("./get_input_data.py " +  fil + " > " + "/tmp/foo")
        fil = "/tmp/foo"

        test_data = np.loadtxt(fil, delimiter=',')
        if (test_data.ndim == 1):
            test_data = np.reshape(test_data, (1, -1))
        
        test_data = test_data[:, 0:(cols - 1)]
        test_data = transform(test_data, False)

        if polynomial:
            poly = PolynomialFeatures(polynomial_number)
            test_data = poly.fit_transform(test_data)

        k = 0
        for clf in classifiers:
            res = clf.fit(actual_train_data, labels)

            ones = len(np.where((res.predict(test_data) == 1))[0])
            zeros = len(np.where((res.predict(test_data) == 0))[0])

            avg[k] = avg[k] + (zeros / (float(zeros) + ones))
            #data.append(str(zeros / (float(zeros) + ones)))
            #rint("Ones: " + str(ones) + ", zeros: " + str(zeros))
            k = k + 1


    for k in range(0, len(avg)):
        avg[k] = avg[k] / 3.
        data.append("{0:.2f}".format(avg[k]))

    to_print.append(data)
    print("===")
    print(to_print)
    print("===")
        
print(tabulate(to_print, classifier_names, tablefmt='orgtbl'))
        #print("============================================")
