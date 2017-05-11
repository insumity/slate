#!/usr/bin/env python2.7
import mmap
import numpy as np
import sys
from sklearn import linear_model, svm, tree, neighbors
from sklearn.neighbors.nearest_centroid import NearestCentroid
from sklearn.linear_model import SGDClassifier
from sklearn.ensemble import RandomForestClassifier

# do the training before hand
train_data = np.loadtxt(sys.argv[1], delimiter=',')
(rows, cols) = train_data.shape
labels = train_data[:, cols - 1]

nn = NearestCentroid()
nn_res = nn.fit(train_data[:, 0:cols - 1], labels)

filename = "/tmp/classifier"
with open(filename, "wb") as f:
    f.write("Hello Python!\n")

with open(filename, "wb") as f:
    mm = mmap.mmap(f.fileno(), 0)
    print mm.readline()  # prints "Hello Python!"
    mm.close()
    
test_data = np.loadtxt(sys.argv[2], delimiter=',')
test_data = test_data[:, 0:(cols - 1)]

print(nn_res.predict(test_data))
