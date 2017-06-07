#!/usr/bin/env python2.7
import numpy as np
import sys
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
    context_switches = data[11]
    bw1 = data[12]
    bw2 = data[13]
    bw3 = data[14]
    bw4 = data[15]
    number_of_threads = data[16]

    intra_socket = l2_miss - (l3_hit + l3_miss)
    inter_socket = remote_fwd + remote_hitm
    bw = bw1 + bw2 + bw3 + bw4
    local_accesses = 0
    if local_dram > 0 or remote_dram > 0:
        local_accesses = local_dram / (1 + local_dram + float(remote_dram))

    normalized = 17 * [0.]
    normalized[0] = LOC
    normalized[1] = RR
    for i in range(2, 11):
        normalized[i] = (float(data[i]) / float(2 * 1000 * 1000 * 1000)) / float(number_of_threads)

    for i in range(11, 16):
        normalized[i] = float(data[i]) / float(number_of_threads)

    normalized[16] = data[16]

    llc_hitrate = l3_hit / float(l3_hit + l3_miss)
    #new_data = np.array([LOC, RR, number_of_threads, bw1, bw2, bw3, bw4, l3_hit, l3_miss, remote_fwd, remote_hitm, uops_retired, unhalted_cycles, local_dram, remote_dram])
    #new_data = np.array([LOC, RR, number_of_threads, bw, uops_retired, unhalted_cycles, (intra_socket + inter_socket) / float(uops_retired),
    #intra_socket / float(1 + l2_miss), inter_socket / float(1 + l3_miss), llc_hitrate, local_accesses])
    
    if printy:
        print("llc hit rate: " + str(llc_hitrate))
        print("intra_socket: " + str(intra_socket / float(l2_miss)))
        print("inter_socket: " + str(inter_socket / float(l3_miss)))

        print("bw: " + str(bw / float(number_of_threads)))
        print("local accesses: " + str(local_accesses))
        print("------")

    return data


def transform(all_data, printy):

    new_all_data = to_new_sample(all_data[0], printy)

    for i in range(1, all_data.shape[0]):
        data = all_data[i]
        new_data = to_new_sample(data, printy)
        new_all_data = np.vstack((new_all_data, new_data))

    return new_all_data





train_data = np.loadtxt(sys.argv[1], delimiter=',')
(rows, cols) = train_data.shape
labels = train_data[:, cols - 1]

def classify(classifier_name, trained_classifier, test_filenames):
    print("============================================")
    print(classifier_name)
    print("============================================")

    for fn in test_filenames:
        print(">>> (" + fn + ")")
        test_data = np.loadtxt(fn, delimiter=',')
        test_data = test_data[:, 0:(cols - 1)]
        test_data = transform(test_data, False)
        #test_data = preprocessing.normalize(test_data, norm='l2')

        #print("actual test data: " + str(test_data))
        poly = PolynomialFeatures(2)
        test_data = poly.fit_transform(test_data)

        print(trained_classifier.predict(test_data))
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
poly = PolynomialFeatures(2)
actual_train_data = poly.fit_transform(actual_train_data)

print("----- About to have a look at test_data -----")


#print(pipeline.fit_transform(train_data[:, 0:cols - 1]))
#print(pipeline.fit_transform(test_data))

#test_data = poly.fit_transform(test_data)


clf = GaussianProcessClassifier()
res = clf.fit(actual_train_data, labels)
classify("gaussian process classifier", res, test_filenames)

clf = GaussianNB()
res = clf.fit(actual_train_data, labels)
classify("gaussian NB", res, test_filenames)

clf = QuadraticDiscriminantAnalysis()
res = clf.fit(actual_train_data, labels)
classify("quadratic discriminant analysis", res, test_filenames)


clf1 = LogisticRegression(random_state=1)
clf2 = SGDClassifier(loss="hinge", penalty="l2", n_iter=5000)
clf = VotingClassifier(estimators=[('rf', clf1), ('lr', clf1)], voting='soft')
res = clf.fit(actual_train_data, labels)
classify("voting classifier", res, test_filenames)

clf = BaggingClassifier()
res = clf.fit(actual_train_data, labels)
classify("bagging classifier", res, test_filenames)

clf = AdaBoostClassifier()
res = clf.fit(actual_train_data, labels)
classify("ada boost", res, test_filenames)

clf = PassiveAggressiveClassifier()
res = clf.fit(actual_train_data, labels)
classify("passive aggressive", res, test_filenames)

print("\nDummy Classifier")
clf = DummyClassifier()
res = clf.fit(actual_train_data, labels)
classify("dummy classifier", res, test_filenames)

clf = RidgeClassifier()
res = clf.fit(actual_train_data, labels)
classify("ridge classifier", res, test_filenames)

logistic = linear_model.LogisticRegression(C=1)
res = logistic.fit(actual_train_data, labels)
classify("logistic regression", res, test_filenames)

clf = MLPClassifier(solver='lbfgs', alpha=1e-5, hidden_layer_sizes=(5), random_state=1)
res = clf.fit(actual_train_data, labels)
classify("mlp classifier", res, test_filenames)

clf = tree.DecisionTreeClassifier(criterion="gini", splitter="random")
res = clf.fit(actual_train_data, labels)
classify("decision tree classifier", res, test_filenames)

clf = tree.DecisionTreeClassifier()
res = clf.fit(actual_train_data, labels)
classify("decision tree classifier - default", res, test_filenames)

nn = NearestCentroid()
res = nn.fit(actual_train_data, labels)
classify("nearest centroid", res, test_filenames)

nn = NearestCentroid(metric="manhattan")
res = nn.fit(actual_train_data, labels)
classify("nearest centroid - manhattan", res, test_filenames)

nc = neighbors.KNeighborsClassifier(15, 'distance')
res = nc.fit(actual_train_data, labels)
classify("k-neighbors", res, test_filenames)

sgd = SGDClassifier(loss="hinge", penalty="l2", n_iter=5000)
res = sgd.fit(actual_train_data, labels)
classify("SGD", res, test_filenames)

sgd = SGDClassifier(loss="modified_huber", penalty="l1", n_iter=6000, alpha=0.001)
res = sgd.fit(actual_train_data, labels)
classify("SGD - modified huber", res, test_filenames)

rf = RandomForestClassifier(n_estimators = 20)
res = rf.fit(actual_train_data, labels)
classify("random forest classifier", res, test_filenames)

