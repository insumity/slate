#!/usr/bin/env python2.7


import mmap, time, struct, os, sys
import numpy as np
from sklearn import linear_model, svm, tree, neighbors
from sklearn.neighbors.nearest_centroid import NearestCentroid
from sklearn.linear_model import SGDClassifier
from sklearn.ensemble import RandomForestClassifier
from sklearn.preprocessing import PolynomialFeatures

# train the model
train_data = np.loadtxt(sys.argv[1], delimiter=',')
(rows, cols) = train_data.shape
labels = train_data[:, cols - 1]

actual_train_data = train_data[:, 0:cols - 1]
print("SHAPE of actual: " + str(actual_train_data.shape))
#poly = PolynomialFeatures(2)
#actual_train_data = poly.fit_transform(actual_train_data)
#print("SHAPE of actual: " + str(actual_train_data.shape))

#logistic = linear_model.LogisticRegression(C=1e5)
logistic = linear_model.LogisticRegression(C=1)
res = logistic.fit(actual_train_data, labels)
#sgd = SGDClassifier(loss="hinge", penalty="l2", n_iter=5000)
#sgd = SGDClassifier(loss="modified_huber", penalty="l1", n_iter=6000, alpha=0.001)
#res = sgd.fit(actual_train_data, labels)

#sgd = SGDClassifier(loss="hinge", penalty="l2", n_iter=5000)
#res = sgd.fit(actual_train_data, labels)
#nc = neighbors.KNeighborsClassifier(15, 'distance')
#res = nc.fit(actual_train_data, labels)

#print(res.predict(test_data))


#nn = SGDClassifier(loss="hinge", penalty="l2", n_iter=5000)
#nn_res = nn.fit(train_data[:, 0:cols - 1], labels)


fmt = "=iiqqqqqqqqqqddddii"

def create_struct(LOC, RR, l3_hit, l3_miss, local_dram, remote_dram, l2_miss, uops_retired,
                  unhalted_cycles, remote_fwd, remote_hitm, context_switches,
                  sockets_bw1, sockets_bw2, sockets_bw3, sockets_bw4, number_of_threads, result):
   return struct.pack(fmt, LOC, RR, l3_hit, l3_miss, local_dram, remote_dram, l2_miss, uops_retired,
                      unhalted_cycles, remote_fwd, remote_hitm, context_switches,
                      sockets_bw1, sockets_bw2, sockets_bw3, sockets_bw4, number_of_threads, result)

    
# write a simple example file
filename = "/tmp/classifier"
with open(filename, "wb+") as f:
   f.write(create_struct(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -0.1, -0.1, -0.1, -0.1, -1, 0))

with open(filename, "r+") as f:

   mm = mmap.mmap(f.fileno(), 0, mmap.MAP_SHARED, mmap.PROT_READ | mmap.PROT_WRITE)
   statinfo = os.stat(filename)

   while True:

      ready = mm.read(statinfo.st_size)
      result = struct.unpack(fmt, ready)

      if result[17] == -1: # needs to classify
         test_data = np.array(result[0:17])
         test_data = np.reshape(test_data, (1, 17))
         print("TEST DATA is ... " + str(test_data))

         #test_data = poly.fit_transform(test_data)
         prediction = res.predict(test_data)
         probabilities = res.predict_proba(test_data)
         print("PROBABILITIES is: " + str(probabilities))
         if probabilities[0][0] >= 0.999999:
            prediction = 0
         elif probabilities[0][1] >= 0.999999:
            prediction = 1
         else:
            prediction = 2 # no choice

         print("The prediction is: " + str(prediction))
         mm.seek(0)
         s = create_struct(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -0.1, -0.1, -0.1, -0.1, -1, prediction)
         print("Size: " + str(struct.calcsize(fmt)))
         mm.write(s)
         mm.flush()

         
      mm.seek(0)
   

      time.sleep(0.02)

   mm.close()
