#!/usr/bin/env python2.7

import numpy as np
import sys
import skflow
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
from sklearn.model_selection import GridSearchCV, cross_val_score


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
    bw4 = data[17]
    number_of_threads = data[17]

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
    new_data = np.array([LOC, RR, intra_socket / float(uops_retired), inter_socket / float(uops_retired), local_dram / float(uops_retired), remote_dram / float(uops_retired),
                         local_dram / float(1 + local_dram + remote_dram), float(remote_fwd + remote_hitm) / float(uops_retired), 
                         instructions / float(unhalted_cycles), uops_retired / float(instructions), l3_hit / float(uops_retired), l3_miss / float(uops_retired),
                         l3_hit / (l3_hit + float(l3_miss)), float(uops_retired) / float(unhalted_cycles), number_of_threads])

    new_data = np.array([LOC, RR, intra_socket / float(uops_retired), inter_socket / float(uops_retired),
                         uops_retired / float(unhalted_cycles), number_of_threads,
                         l3_hit / (l3_hit + float(l3_miss)), l3_miss / float(uops_retired), (local_dram + remote_dram) / float(uops_retired),
                         4 * bw if RR else bw, instructions / float(uops_retired) ])
     
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
            
        # print("number of threads: " + str(number_of_threads))
        # print("llc hit rate: " + str(llc_hitrate))
        # print("intra_socket: " + str(intra_socket / float(uops_retired)))
        # print("inter_socket: " + str(inter_socket / float(uops_retired)))
        # print("local: " + str(local_dram / float(uops_retired)))
        # print("remote: " + str(remote_dram / float(uops_retired)))
        # print("bw: " + str((bw) / float(unhalted_cycles) ))
        print("number ol threads: " + str(number_of_threads))
        print("llc hit rate: " + str(llc_hitrate))
        print("intra socket: " + str(intra_socket / float(uops_retired)))
        print("inter socket: " + str(inter_socket / float(uops_retired)))
        print("uops retired per cycle: " + str(uops_retired / float(unhalted_cycles)))
        print("llcc miss per uops retired: " + str(l3_miss / float(uops_retired)))
        print("bw1, bw2, bw3, bw4: " + str(bw1) + " " + str(bw2) + " " + str(bw3) + " " + str(bw4))
        print("(local + remote dram): " + str((local_dram + remote_dram) / float(uops_retired)))
        #print("bw: " +str((bw) / float(unhalted_cycles) ))

        
        print(new_data)
        print(data)
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




actual_train_data=[]
labels=[]

train_data = np.loadtxt(sys.argv[1], delimiter=',')
(rows, cols) = train_data.shape
labels = train_data[:, cols - 1]

def classify(classifier_name, trained_classifier, test_filenames):
    print("============================================")
    print(classifier_name)
    print("============================================")

    # parameters = {'n_estimators':[5, 10, 20, 30, 40, 50, 100], 'criterion':('gini', 'entropy'), 'min_samples_split':[0.1, 0.2, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9],
    #               'min_samples_leaf':[0.01, 0.1, 0.2, 0.4]}
    # clf = GridSearchCV(trained_classifier, parameters)
    # clf.fit(actual_train_data, labels)
    # print(clf.best_params_)

    scores = cross_val_score(trained_classifier, actual_train_data, labels, cv=10)
    print("Accuracy: %0.2f (+/- %0.2f)" % (scores.mean(), scores.std() * 2))


    for fn in test_filenames:
        print(">>> (" + fn + ")")
        test_data = np.loadtxt(fn, delimiter=',')
        test_data = test_data[:, 0:(cols - 1)]
        test_data = transform(test_data, False)
        #test_data = preprocessing.normalize(test_data, norm='l2')


        #print("actual test data: " + str(test_data))
        if polynomial:
            poly = PolynomialFeatures(polynomial_number)
            test_data = poly.fit_transform(test_data)

        print(trained_classifier.predict(test_data))
        print(trained_classifier.predict_proba(test_data))
        ones = len(np.where((trained_classifier.predict(test_data) == 1))[0])
        zeros = len(np.where((trained_classifier.predict(test_data) == 0))[0])
        print("Result: " + str(ones) + "/" + str(zeros))
        print(np.where((trained_classifier.predict(test_data) == labels) == False))

        
    print("============================================")


test_filenames = []
for i in range(2, len(sys.argv)):
    test_filenames.append(sys.argv[i])


actual_train_data = train_data[:, 0:cols - 1]
print("SHAPE of actual: " + str(actual_train_data.shape))
#print("SHAPE of actual: " + str(transform(train_data[:, 0:cols - 1], False).shape))
#print(transform(train_data[:, 0:cols - 1], False))


pipeline = Pipeline([('scaling', StandardScaler()), ('pca', PCA(n_components=3))])

actual_train_data = transform(train_data[:, 0:cols - 1], False)
#actual_train_data = preprocessing.normalize(actual_train_data, norm='l2')

#print("actual train data: " + str(actual_train_data[0]))

if polynomial:
    poly = PolynomialFeatures(polynomial_number)
    actual_train_data = poly.fit_transform(actual_train_data)

print("----- About to have a look at test_data -----")


#print(pipeline.fit_transform(train_data[:, 0:cols - 1]))
#print(pipeline.fit_transform(test_data))

#test_data = poly.fit_transform(test_data)


# clf = GaussianProcessClassifier()
# res = clf.fit(actual_train_data, labels)
# classify("gaussian process classifier", res, test_filenames)

# clf = GaussianNB()
# res = clf.fit(actual_train_data, labels)
# classify("gaussian NB", res, test_filenames)

rf = RandomForestClassifier(random_state=20)
res = rf.fit(actual_train_data, labels)
classify("random forest classifier", res, test_filenames)


rf = RandomForestClassifier()
res = rf.fit(actual_train_data, labels)
classify("random forest classifier no random state", res, test_filenames)



rf = RandomForestClassifier(n_estimators=30, criterion="gini", min_samples_split=0.1, min_samples_leaf=0.01)
res = rf.fit(actual_train_data, labels)
classify("random forest classifier no random state", res, test_filenames)


rf = RandomForestClassifier(n_estimators=1000, criterion="entropy", min_samples_split=0.1, min_samples_leaf=0.01, oob_score=True, max_features=None)
res = rf.fit(actual_train_data, labels)
classify("random forest classifier no random state", res, test_filenames)

