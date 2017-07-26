#!/usr/bin/env python2.7
import mmap, time, struct, os, sys
import numpy as np

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

polynomial = False
polynomial_number = 3

def to_new_sample(data):
    LOC = data[0] # tick
    RR = data[1]  # tick
    l3_hit = data[2] # tick
    l3_miss = data[3] # tick
    local_dram = data[4] # tick 
    arith_divider_uops = data[5]
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

    #data =  np.hstack((data[0:5], data[6:12], data[17]))

    data =  np.hstack((data[0:2], data[6:9], data[10:12], data[17]))

    data = data.reshape(1, -1)
    if polynomial:
       poly = PolynomialFeatures(polynomial_number)
       return poly.fit_transform(data)

    return data

def hto_new_sample(data):
    LOC = data[0] # tick
    RR = data[1]  # tick
    l3_hit = data[2] # tick
    l3_miss = data[3] # tick
    local_dram = data[4] # tick 
    arith_divider_uops = data[5]
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


    #data =  np.hstack((data[2:4], data[6], data[5], data[7:9], data[11], data[17]))
    data =  np.hstack((data[5] / float(uops_retired), uops_retired / float(unhalted_cycles), (l2_miss - (l3_hit + l3_miss)) / float(uops_retired) , data[17]))
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


def htransform(all_data):
    new_all_data = hto_new_sample(all_data[0])

    for i in range(1, all_data.shape[0]):
        data = all_data[i]
        new_data = hto_new_sample(data)
        new_all_data = np.vstack((new_all_data, new_data))

    return new_all_data




htrain_data = np.loadtxt("hyperthreads_or_not.csv", delimiter=',')
(_, hcols) = htrain_data.shape
hlabels = htrain_data[:, hcols - 1]
hactual_train_data = htransform(htrain_data[:, 0:hcols - 1])
if polynomial:
    poly = PolynomialFeatures(polynomial_number)
    hactual_train_data = poly.fit_transform(hactual_train_data)


def to_hyperthread(test_data):
    result = ""
    clf = QuadraticDiscriminantAnalysis()
    res = clf.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    clf1 = LogisticRegression(random_state=1)
    clf2 = SGDClassifier(loss="hinge", penalty="l2", n_iter=5000)
    clf = VotingClassifier(estimators=[('rf', clf1), ('lr', clf1)], voting='soft')
    res = clf.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)) + ", "

    clf = BaggingClassifier()
    res = clf.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    clf = AdaBoostClassifier(n_estimators=100)
    res = clf.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    clf = PassiveAggressiveClassifier()
    res = clf.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    clf = DummyClassifier()
    res = clf.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    clf = RidgeClassifier()
    res = clf.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    logistic = linear_model.LogisticRegression(C=0.0001)
    res = logistic.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "


    logistic = linear_model.LogisticRegression()
    res = logistic.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    clf = MLPClassifier(solver='lbfgs', alpha=1e-5, hidden_layer_sizes=(5), random_state=1)
    res = clf.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    clf = MLPClassifier(solver='lbfgs', activation='logistic', alpha=1e-5, hidden_layer_sizes=(12), random_state=1)
    res = clf.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    clf = tree.DecisionTreeClassifier(criterion="gini", splitter="random")
    res = clf.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    clf = tree.DecisionTreeClassifier()
    res = clf.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    nn = NearestCentroid()
    res = nn.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    nn = NearestCentroid(metric="manhattan")
    res = nn.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    nc = neighbors.KNeighborsClassifier(15, 'distance')
    res = nc.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    sgd = SGDClassifier(loss="hinge", penalty="l2", n_iter=5000)
    res = sgd.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    sgd = SGDClassifier(loss="modified_huber", penalty="l1", n_iter=6000, alpha=0.001)
    res = sgd.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    rf = RandomForestClassifier(random_state=20)
    res = rf.fit(hactual_train_data, hlabels)
    result = result + "{{{ " + str(res.predict(test_data)[0]) + "}}} , "

    rf = RandomForestClassifier(oob_score=True, n_estimators=50)
    res = rf.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    rf = RandomForestClassifier(oob_score=True, n_estimators=50, max_features=None)
    res = rf.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    clf = LogisticRegression(random_state=1)
    res = clf.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    clf = tree.DecisionTreeClassifier(criterion="gini", splitter="random")
    res = clf.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    clf = tree.DecisionTreeClassifier()
    res = clf.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "


    res = nn.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    nn = NearestCentroid(metric="manhattan")
    res = nn.fit(hactual_train_data, hlabels)
    result = result + str(res.predict(test_data)[0]) + ", "

    return result



#pipe = Pipeline([('std_scaler', StandardScaler())])

# train the model
train_data = np.loadtxt(sys.argv[1], delimiter=',')
(rows, cols) = train_data.shape
labels = train_data[:, cols - 1]


always_predict = -1
if len(sys.argv) == 3:
    always_predict = int(sys.argv[2])
    if always_predict != 0 and always_predict != 1:
        print("Prediction can only be 0 or 1")
        exit()



actual_train_data = transform(train_data[:, 0:cols - 1])
#actual_train_data = pipe.fit_transform(actual_train_data)

#poly = PolynomialFeatures(2)
#actual_train_data = poly.fit_transform(actual_train_data)
#print("SHAPE of actual: " + str(actual_train_data.shape))

#logistic = linear_model.LogisticRegression(C=1e5)
#logistic = linear_model.LogisticRegression(C=1)
#res = logistic.fit(hactual_train_data, hlabels)

logistic = NearestCentroid() 
res = logistic.fit(actual_train_data, labels)
print(" --- Model was trained ---")
if always_predict != -1:
    print("Always predicting: " + str(always_predict))

#sgd = SGDClassifier(loss="hinge", penalty="l2", n_iter=5000)
#sgd = SGDClassifier(loss="modified_huber", penalty="l1", n_iter=6000, alpha=0.001)
#res = sgd.fit(hactual_train_data, hlabels)

#sgd = SGDClassifier(loss="hinge", penalty="l2", n_iter=5000)
#res = sgd.fit(hactual_train_data, hlabels)
#nc = neighbors.KNeighborsClassifier(15, 'distance')
#res = nc.fit(hactual_train_data, hlabels)

#print(res.predict(test_data)[0])


#nn = SGDClassifier(loss="hinge", penalty="l2", n_iter=5000)
#nn_res = nn.fit(train_data[:, 0:cols - 1], labels)


fmt = "=iiqqqqqqqqqqqddddii"

def create_struct(LOC, RR, l3_hit, l3_miss, local_dram, arith_divider_uops, l2_miss, uops_retired,
                  unhalted_cycles, remote_fwd, remote_hitm, instructions, context_switches,
                  sockets_bw1, sockets_bw2, sockets_bw3, sockets_bw4, number_of_threads, result):
   return struct.pack(fmt, LOC, RR, l3_hit, l3_miss, local_dram, arith_divider_uops, l2_miss, uops_retired,
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
         htest_data = np.array(result[0:18])


         number_of_threads = test_data[17]
         test_data = transform(np.reshape(test_data, (1, 18)))
         #test_data = pipe.fit_transform(test_data)

         #print("TEST DATA is ... " + str(test_data))

         #test_data = poly.fit_transform(test_data)
         if always_predict == -1:
             prediction = res.predict(test_data)
         else:
             prediction = always_predict
             
         #print(test_data)
         print("The prediction is: " + str(prediction) + " (#threads = " + str(number_of_threads) + ")")
         #+ "... and " + str(to_hyperthread(htransform(np.reshape(htest_data, (1, 18))))))
         
         mm.seek(0)
         s = create_struct(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -0.1, -0.1, -0.1, -0.1, -1, prediction)
         #print("Size: " + str(struct.calcsize(fmt)))
         mm.write(s)
         mm.flush()

         
      mm.seek(0)
   

      time.sleep(0.02)

   mm.close()
