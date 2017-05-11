#!/usr/bin/env python2.7

import numpy as np
import sys
from sklearn import linear_model, svm, tree, neighbors
from sklearn.neighbors.nearest_centroid import NearestCentroid
from sklearn.linear_model import SGDClassifier
from sklearn.ensemble import RandomForestClassifier

train_data = np.loadtxt(sys.argv[1], delimiter=',')
(rows, cols) = train_data.shape
labels = train_data[:, cols - 1]

test_data = np.loadtxt(sys.argv[2], delimiter=',')
test_data = test_data[:, 0:(cols - 1)]
print("TEST data shape: " + str(test_data.shape))

print(str(train_data.shape) + ": " + str(test_data.shape))
logistic = linear_model.LogisticRegression(C=1e5)
res = logistic.fit(train_data[:, 0:cols - 1], labels)
print(res.predict(test_data))

print("====")
svc = svm.SVC(kernel='linear')
sv_res = svc.fit(train_data[:, 0:cols - 1], labels)
print(sv_res.predict(test_data))

# print("====")
# svc = svm.SVC(kernel='poly', degree=2)
# sv_res = svc.fit(train_data[:, 0:cols - 1], labels)
# print(sv_res.predict(test_data))

# print("====")
# clf = tree.DecisionTreeClassifier()
# clf_res = clf.fit(train_data[:, 0:cols - 1], labels)
# print(clf_res.predict(test_data))

print("==== nearest centroid ... what is happening!")
nn = NearestCentroid()
nn_res = nn.fit(train_data[:, 0:cols - 1], labels)
print("test_data: " + str(test_data))
print(nn_res.predict(test_data))

print("==== nearest neighbors")
nc = neighbors.KNeighborsClassifier(15, 'distance')
nc_res = nc.fit(train_data[:, 0:cols - 1], labels)
print(nc_res.predict(test_data))

print("====")
sgd = SGDClassifier(loss="hinge", penalty="l2", n_iter=5000)
sgd_res = sgd.fit(train_data[:, 0:cols - 1], labels)
print(sgd_res.predict(test_data))

# print("====")
# rf = RandomForestClassifier(n_estimators=10)
# rf_res = rf.fit(train_data[:, 0:cols - 1], labels)
# print(rf_res.predict(test_data))
