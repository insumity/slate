#!/usr/bin/env python2.7

import numpy as np
import sys
from sklearn import linear_model, svm, tree, neighbors
from sklearn.neighbors.nearest_centroid import NearestCentroid
from sklearn.linear_model import SGDClassifier
from sklearn.ensemble import RandomForestClassifier
from sklearn.neural_network import MLPClassifier

train_data = np.loadtxt(sys.argv[1], delimiter=',')
(rows, cols) = train_data.shape
labels = train_data[:, cols - 1]

test_data = np.loadtxt(sys.argv[2], delimiter=',')
test_data = test_data[:, 0:(cols - 1)]


print("\nlogistic regression")
logistic = linear_model.LogisticRegression(C=1e5)
res = logistic.fit(train_data[:, 0:cols - 1], labels)
print(res.predict(test_data))
print(res.predict_proba(test_data))

print("\nlogistic regression - less C ")
logistic = linear_model.LogisticRegression(penalty='l1', C=1e5)
res = logistic.fit(train_data[:, 0:cols - 1], labels)
print(res.predict(test_data))
print(res.predict_proba(test_data))

print("\nmlpclassifier")
clf = MLPClassifier(solver='lbfgs', alpha=1e-5, hidden_layer_sizes=(12, 3), random_state=1)
res = clf.fit(train_data[:, 0:cols - 1], labels)
print(res.predict(test_data))

print("\nsupport vector machines - linear kernel")
svc = svm.SVC(kernel='linear')
sv_res = svc.fit(train_data[:, 0:cols - 1], labels)
print(sv_res.predict(test_data))

print("\nsupport vector machines - polynomial kernel")
svc = svm.SVC(kernel='poly', degree=3)
sv_res = svc.fit(train_data[:, 0:cols - 1], labels)
print(sv_res.predict(test_data))

print("\ndecision tree classifier")
clf = tree.DecisionTreeClassifier(criterion="gini", splitter="random")
clf_res = clf.fit(train_data[:, 0:cols - 1], labels)
print(clf_res.predict(test_data))
print(clf_res.predict_proba(test_data))

print("\ndecision tree classifier - default")
clf = tree.DecisionTreeClassifier()
clf_res = clf.fit(train_data[:, 0:cols - 1], labels)
print(clf_res.predict(test_data))
print(clf_res.predict_proba(test_data))

print("\nnearest centroid")
nn = NearestCentroid()
nn_res = nn.fit(train_data[:, 0:cols - 1], labels)
#print("test_data: " + str(test_data))
print(nn_res.predict(test_data))

print("\nnearest centroid - manhattan ")
nn = NearestCentroid(metric="manhattan")
nn_res = nn.fit(train_data[:, 0:cols - 1], labels)
print(nn_res.predict(test_data))

print("\nnearest neighbors")
nc = neighbors.KNeighborsClassifier(15, 'distance')
nc_res = nc.fit(train_data[:, 0:cols - 1], labels)
print(nc_res.predict(test_data))

print("\nSGD classifier")
sgd = SGDClassifier(loss="hinge", penalty="l2", n_iter=5000)
sgd_res = sgd.fit(train_data[:, 0:cols - 1], labels)
print(sgd_res.predict(test_data))

print("\nrandom forest classifier")
rf = RandomForestClassifier()
rf_res = rf.fit(train_data[:, 0:cols - 1], labels)
print(rf_res.predict(test_data))
print(rf_res.predict_proba(test_data))
