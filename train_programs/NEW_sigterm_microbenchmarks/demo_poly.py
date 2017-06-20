#!/usr/bin/env python2.7


import mmap, time, struct, os, sys
import numpy as np
from sklearn import linear_model, svm, tree, neighbors
from sklearn.neighbors.nearest_centroid import NearestCentroid
from sklearn.linear_model import SGDClassifier
from sklearn.ensemble import RandomForestClassifier
from sklearn.preprocessing import PolynomialFeatures
from sklearn.linear_model import *
from sklearn.ensemble import *
from sklearn.dummy import *


polynomial = False
polynomial_number = 3

def to_new_sample(data):
    LOC = data[0] # tick
    RR = data[1]  # tick
    l3_hit = data[2] # tick
    l3_miss = data[3] # tick
    local_dram = data[4] # tick 
    remote_dram = data[5]
    l2_miss = data[6] # tick
    uops_retired = data[7] # tick
    unhalted_cycles = data[8] # tick
    remote_fwd = data[9] # tick
    remote_hitm = data[10] # tick
    instructions = data[11] # tick
    context_switches = data[12]
    bw1 = data[13]
    bw2 = data[14]
    bw3 = data[15]
    bw4 = data[16]
    number_of_threads = data[17]

    data =  np.hstack((data[0:5], data[6:12], data[17]))
    data = data.reshape(1, -1)
    if polynomial:
       poly = PolynomialFeatures(polynomial_number)
       return poly.fit_transform(data)

    return data

def transform(all_data):

    new_all_data = to_new_sample(all_data[0])

    for i in range(1, all_data.shape[0]):
        data = all_data[i]
        new_data = to_new_sample(data)
        new_all_data = np.vstack((new_all_data, new_data))

    return new_all_data




# train the model
train_data = np.loadtxt(sys.argv[1], delimiter=',')
(rows, cols) = train_data.shape
labels = train_data[:, cols - 1]

actual_train_data = transform(train_data[:, 0:cols - 1])
#poly = PolynomialFeatures(2)
#actual_train_data = poly.fit_transform(actual_train_data)
#print("SHAPE of actual: " + str(actual_train_data.shape))

#logistic = linear_model.LogisticRegression(C=1e5)
#logistic = linear_model.LogisticRegression(C=1)
#res = logistic.fit(actual_train_data, labels)

logistic = NearestCentroid(metric="manhattan")
res = logistic.fit(actual_train_data, labels)
print(" --- Model was trained ---")

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


fmt = "=iiqqqqqqqqqqqddddii"

def create_struct(LOC, RR, l3_hit, l3_miss, local_dram, remote_dram, l2_miss, uops_retired,
                  unhalted_cycles, remote_fwd, remote_hitm, instructions, context_switches,
                  sockets_bw1, sockets_bw2, sockets_bw3, sockets_bw4, number_of_threads, result):
   return struct.pack(fmt, LOC, RR, l3_hit, l3_miss, local_dram, remote_dram, l2_miss, uops_retired,
                      unhalted_cycles, remote_fwd, remote_hitm, instructions, context_switches,
                      sockets_bw1, sockets_bw2, sockets_bw3, sockets_bw4, number_of_threads, result)

    
# write a simple example file
filename = "/tmp/classifier"
with open(filename, "wb+") as f:
   f.write(create_struct(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -0.1, -0.1, -0.1, -0.1, -1, 0))

with open(filename, "r+") as f:

   mm = mmap.mmap(f.fileno(), 0, mmap.MAP_SHARED, mmap.PROT_READ | mmap.PROT_WRITE)
   statinfo = os.stat(filename)

   while True:

      ready = mm.read(statinfo.st_size)
      result = struct.unpack(fmt, ready)

      if result[18] == -1: # needs to classify
         test_data = np.array(result[0:18])
         test_data = transform(np.reshape(test_data, (1, 18)))
         #print("TEST DATA is ... " + str(test_data))

         #test_data = poly.fit_transform(test_data)
         prediction = res.predict(test_data)
         # //probabilities = res.predict_proba(test_data)
         # print("PROBABILITIES is: " + str(probabilities))
         # if probabilities[0][0] >= 0.999999:
         #    prediction = 0
         # elif probabilities[0][1] >= 0.999999:
         #    prediction = 1
         # else:
         #    prediction = 2 # no choice


         print("The prediction is: " + str(prediction))
         mm.seek(0)
         s = create_struct(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -0.1, -0.1, -0.1, -0.1, -1, prediction)
         print("Size: " + str(struct.calcsize(fmt)))
         mm.write(s)
         mm.flush()

         
      mm.seek(0)
   

      time.sleep(0.02)

   mm.close()
