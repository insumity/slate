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
    #f = np.hstack((data[0:12], data[17]))
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


clf1 = QuadraticDiscriminantAnalysis()
clf2 = MLPClassifier(random_state=42)
clf3 = BaggingClassifier(random_state=42)
eclf = VotingClassifier(estimators=[('qda', clf1), ('nc', clf2), ('bc', clf3)], voting="soft", weights=[1, 1, 1])


qda = QuadraticDiscriminantAnalysis()
dtc = tree.DecisionTreeClassifier(random_state=42)
rfc = RandomForestClassifier(random_state=42)
abc = AdaBoostClassifier(random_state=42)
classifiers = [qda, LogisticRegression(random_state=42), BaggingClassifier(random_state=42), abc,
               PassiveAggressiveClassifier(random_state=42), DummyClassifier(random_state=42), RidgeClassifier(random_state=42), MLPClassifier(random_state=42),
               dtc, NearestCentroid(), neighbors.KNeighborsClassifier(), SGDClassifier(random_state=42),
               rfc, eclf]

real_classifier_names = ["Quadratic", "Logistic", "Bagging", "adaboost", "Passive", "Dummy", "Ridge", "MLPClassifier", "decision tree", "nearest centroid", "kneighbours", "sgd", "random forest classifier",
                                             "voting classif"]

classifier_names = ["Application", "#threads", "(executed with) Policy", "Time (s) loc", "Time (s) mem", "Difference between loc and mem (perc)?", "Perc >= 0%",
                                        "Time (s) Linux", "Time(s) Random -Hwcs", "Time (s) Random - Cores", "loc vs mem?",
                                        "QuadraticDiscriminantAnalysis", "temp", "LogisticRegressin", "temp", "Bagging", "temp", "AdaBoost", "temp", "PassiveAggressive", "temp",
                                        "Dummy", "temp", "Ridge", "temp", "MLP", "temp", "DecisionTree", "temp",
                                        "NearestCentroid", "temp", "KNeighbors", "temp", "SGD", "temp", "RandomForest", "temp", "VotingClassifier", "temp"]



# benchmarks from Metis (PCA & string_match are taken from the new version of Metis)
pca_times = {}
pca_times[6] = {}
pca_times[10] = {}
pca_times[20] = {}

hist_times = {}
hist_times[6] = {}
hist_times[10] = {}
hist_times[20] = {}

kmeans_times = {}
kmeans_times[6] = {}
kmeans_times[10] = {}
kmeans_times[20] = {}

stringmatch_times = {}
stringmatch_times[6] = {}
stringmatch_times[10] = {}
stringmatch_times[20] = {}

linearregression_times = {}
linearregression_times[6] = {}
linearregression_times[10] = {}
linearregression_times[20] = {}

wc_times = {}
wc_times[6] = {}
wc_times[10] = {}
wc_times[20] = {}

matrixmult_times = {}
matrixmult_times[6] = {}
matrixmult_times[10] = {}
matrixmult_times[20] = {}

wr_times = {}
wr_times[6] = {}
wr_times[10] = {}
wr_times[20] = {}

wrmem_times = {}
wrmem_times[6] = {}
wrmem_times[10] = {}
wrmem_times[20] = {}

blackscholes_times = {}
blackscholes_times[6] = {}
blackscholes_times[10] = {}
blackscholes_times[20] = {}

bodytrack_times = {}
bodytrack_times[6] = {}
bodytrack_times[10] = {}
bodytrack_times[20] = {}

canneal_times = {}
canneal_times[6] = {}
canneal_times[10] = {}
canneal_times[20] = {}

facesim_times = {}
facesim_times[4] = {}
facesim_times[8] = {}
facesim_times[16] = {}

ferret_times = {}
ferret_times[6] = {}
ferret_times[10] = {}
ferret_times[22] = {}

fluidanimate_times = {}
fluidanimate_times[4] = {}
fluidanimate_times[8] = {}
fluidanimate_times[16] = {}

raytrace_times = {}
raytrace_times[6] = {}
raytrace_times[10] = {}
raytrace_times[20] = {}

sc_times = {}
sc_times[6] = {}
sc_times[10] = {}
sc_times[20] = {}

sw_times = {}
sw_times[6] = {}
sw_times[10] = {}
sw_times[20] = {}

vips_times = {}
vips_times[6] = {}
vips_times[10] = {}
vips_times[20] = {}

kyotocabinet_times = {}
kyotocabinet_times[6] = {}
kyotocabinet_times[10] = {}
kyotocabinet_times[20] = {}

upscaledb_times = {}
upscaledb_times[6] = {}
upscaledb_times[10] = {}
upscaledb_times[20] = {}

upscaledb_times[6]["loc"] = 7.5961606
upscaledb_times[6]["loc_std"] = 0.11269653272146397
upscaledb_times[10]["loc"] = 10.0998656
upscaledb_times[10]["loc_std"] = 0.04351210859795236
upscaledb_times[20]["loc"] = 17.244105
upscaledb_times[20]["loc_std"] = 0.13861304309046824
upscaledb_times[6]["mem"] = 10.956476
upscaledb_times[6]["mem_std"] = 0.0969013007941586
upscaledb_times[10]["mem"] = 15.127225
upscaledb_times[10]["mem_std"] = 0.0956218230259181
upscaledb_times[20]["mem"] = 17.8296214
upscaledb_times[20]["mem_std"] = 0.12715964454275577
upscaledb_times[6]["linux"] = 9.023169
upscaledb_times[6]["linux_std"] = 0.22628460758345895
upscaledb_times[10]["linux"] = 13.2573582
upscaledb_times[10]["linux_std"] = 0.5683192785740776
upscaledb_times[20]["linux"] = 14.8507842
upscaledb_times[20]["linux_std"] = 0.09963396036773807
upscaledb_times[6]["slate"] = 8.4675808
upscaledb_times[6]["slate_std"] = 0.06281389001614213
upscaledb_times[10]["slate"] = 11.1330398
upscaledb_times[10]["slate_std"] = 0.2517279145747646
upscaledb_times[20]["slate"] = 17.275496
upscaledb_times[20]["slate_std"] = 0.14833632518435935



kyotocabinet_times[6]["slate"] = 6.313157
kyotocabinet_times[6]["slate_std"] = 0.18618948593623647
kyotocabinet_times[10]["slate"] = 11.5647862
kyotocabinet_times[10]["slate_std"] = 0.11870939692947648
kyotocabinet_times[20]["slate"] = 43.1326602
kyotocabinet_times[20]["slate_std"] = 0.3769825621799502
kyotocabinet_times[6]["linux"] = 5.7884112
kyotocabinet_times[6]["linux_std"] = 0.24775687312637767
kyotocabinet_times[10]["linux"] = 11.4149504
kyotocabinet_times[10]["linux_std"] = 0.1652920261048306
kyotocabinet_times[20]["linux"] = 26.252795
kyotocabinet_times[20]["linux_std"] = 1.3469767258216454
kyotocabinet_times[6]["loc"] = 5.029865
kyotocabinet_times[6]["loc_std"] = 0.16095773283691592
kyotocabinet_times[10]["loc"] = 10.2545748
kyotocabinet_times[10]["loc_std"] = 0.2752938046134711
kyotocabinet_times[20]["loc"] = 42.0603664
kyotocabinet_times[20]["loc_std"] = 1.183945791762883
kyotocabinet_times[6]["mem"] = 10.8361512
kyotocabinet_times[6]["mem_std"] = 0.2014689461136877
kyotocabinet_times[10]["mem"] = 20.1768884
kyotocabinet_times[10]["mem_std"] = 0.43948563616418684
kyotocabinet_times[20]["mem"] = 50.8889762
kyotocabinet_times[20]["mem_std"] = 0.7343291827384774


pca_times[6]["loc"] = 3.6431588
pca_times[6]["loc_std"] = 0.10343447346296109
pca_times[10]["loc"] = 3.2623908
pca_times[10]["loc_std"] = 0.016480179275723914
pca_times[20]["loc"] = 5.3348976
pca_times[20]["loc_std"] = 0.2929272969483042
hist_times[6]["loc"] = 18.8650678
hist_times[6]["loc_std"] = 0.11747828026899271
hist_times[10]["loc"] = 12.0194146
hist_times[10]["loc_std"] = 0.014485089141596609
hist_times[20]["loc"] = 7.5871438
hist_times[20]["loc_std"] = 0.5925568602824205
kmeans_times[6]["loc"] = 26.7824796
kmeans_times[6]["loc_std"] = 0.01003850097574334
kmeans_times[10]["loc"] = 16.4251328
kmeans_times[10]["loc_std"] = 0.035045606751203495
kmeans_times[20]["loc"] = 8.7561738
kmeans_times[20]["loc_std"] = 0.04546451651738969
stringmatch_times[6]["loc"] = 29.2098148
stringmatch_times[6]["loc_std"] = 0.06996813353634639
stringmatch_times[10]["loc"] = 17.8943234
stringmatch_times[10]["loc_std"] = 0.03401978050840423
stringmatch_times[20]["loc"] = 9.576033
stringmatch_times[20]["loc_std"] = 0.06193773140178772
linearregression_times[6]["loc"] = 8.3459324
linearregression_times[6]["loc_std"] = 0.010618962747839357
linearregression_times[10]["loc"] = 5.3781692
linearregression_times[10]["loc_std"] = 0.013896688258718334
linearregression_times[20]["loc"] = 3.3750404
linearregression_times[20]["loc_std"] = 0.0538724823953751
pca_times[6]["mem"] = 5.6713664
pca_times[6]["mem_std"] = 0.2835638605341661
pca_times[10]["mem"] = 5.7065196
pca_times[10]["mem_std"] = 0.13692653662544743
pca_times[20]["mem"] = 7.7181128
pca_times[20]["mem_std"] = 0.3577958971510993
hist_times[6]["mem"] = 17.4108182
hist_times[6]["mem_std"] = 0.07789114438086014
hist_times[10]["mem"] = 11.6371212
hist_times[10]["mem_std"] = 0.02291825740670525
hist_times[20]["mem"] = 7.0613702
hist_times[20]["mem_std"] = 0.04897270337810646
kmeans_times[6]["mem"] = 24.8800038
kmeans_times[6]["mem_std"] = 0.2610340827236168
kmeans_times[10]["mem"] = 16.0297604
kmeans_times[10]["mem_std"] = 0.03192033013989674
kmeans_times[20]["mem"] = 8.8823982
kmeans_times[20]["mem_std"] = 0.1510761549039424
stringmatch_times[6]["mem"] = 26.4421144
stringmatch_times[6]["mem_std"] = 0.04004370896208292
stringmatch_times[10]["mem"] = 16.5078972
stringmatch_times[10]["mem_std"] = 0.010506399980963983
stringmatch_times[20]["mem"] = 9.527443
stringmatch_times[20]["mem_std"] = 0.031183737678475938
linearregression_times[6]["mem"] = 7.7253976
linearregression_times[6]["mem_std"] = 0.02925722370697534
linearregression_times[10]["mem"] = 5.2050932
linearregression_times[10]["mem_std"] = 0.008522789834320685
linearregression_times[20]["mem"] = 3.3423676
linearregression_times[20]["mem_std"] = 0.046457039563880954
wc_times[6]["loc"] = 8.4803188
wc_times[6]["loc_std"] = 0.047574532335694066
wc_times[10]["loc"] = 8.2913606
wc_times[10]["loc_std"] = 0.08295840810333814
wc_times[20]["loc"] = 15.752209
wc_times[20]["loc_std"] = 0.752535805915971
matrixmult_times[6]["loc"] = 8.1396202
matrixmult_times[6]["loc_std"] = 0.02082711009621834
matrixmult_times[10]["loc"] = 5.0171986
matrixmult_times[10]["loc_std"] = 0.00808888695927938
matrixmult_times[20]["loc"] = 2.880944
matrixmult_times[20]["loc_std"] = 0.03828875835019987
wr_times[6]["loc"] = 11.1261434
wr_times[6]["loc_std"] = 0.08221243804583343
wr_times[10]["loc"] = 11.2367594
wr_times[10]["loc_std"] = 0.07289674779192827
wr_times[20]["loc"] = 16.1314396
wr_times[20]["loc_std"] = 0.23775885673564298
wrmem_times[6]["loc"] = 22.2087184
wrmem_times[6]["loc_std"] = 0.05163285396179452
wrmem_times[10]["loc"] = 15.6612584
wrmem_times[10]["loc_std"] = 0.07039654105309437
wrmem_times[20]["loc"] = 12.3407384
wrmem_times[20]["loc_std"] = 0.10068150197648026
blackscholes_times[6]["loc"] = 5.0063288
blackscholes_times[6]["loc_std"] = 0.003341489931153467
blackscholes_times[10]["loc"] = 3.9067994
blackscholes_times[10]["loc_std"] = 0.02176840257437371
blackscholes_times[20]["loc"] = 3.1846706
blackscholes_times[20]["loc_std"] = 0.0426425425583419
bodytrack_times[6]["loc"] = 46.9472258
bodytrack_times[6]["loc_std"] = 0.15154873919686696
bodytrack_times[10]["loc"] = 34.723702
bodytrack_times[10]["loc_std"] = 0.3939054577174579
bodytrack_times[20]["loc"] = 25.1315008
bodytrack_times[20]["loc_std"] = 0.23320483040657627
canneal_times[6]["loc"] = 28.1586418
canneal_times[6]["loc_std"] = 0.34934135551257023
canneal_times[10]["loc"] = 18.8357138
canneal_times[10]["loc_std"] = 0.004889092242942446
canneal_times[20]["loc"] = 13.902081
canneal_times[20]["loc_std"] = 0.04918350545863928
facesim_times[4]["loc"] = 28.9041218
facesim_times[4]["loc_std"] = 0.06691491354668255
facesim_times[8]["loc"] = 19.2018522
facesim_times[8]["loc_std"] = 0.23527861963416905
facesim_times[16]["loc"] = 18.2441854
facesim_times[16]["loc_std"] = 0.15702892689896342
ferret_times[6]["loc"] = 73.220191
ferret_times[6]["loc_std"] = 0.12323622781795944
ferret_times[10]["loc"] = 38.5505132
ferret_times[10]["loc_std"] = 0.05221816248165
ferret_times[22]["loc"] = 15.922714
ferret_times[22]["loc_std"] = 0.043858967219942604
fluidanimate_times[4]["loc"] = 38.6445484
fluidanimate_times[4]["loc_std"] = 0.5763118006907719
fluidanimate_times[8]["loc"] = 22.0817536
fluidanimate_times[8]["loc_std"] = 0.03828687178446419
fluidanimate_times[16]["loc"] = 13.5521038
fluidanimate_times[16]["loc_std"] = 0.10058394939432434
raytrace_times[6]["loc"] = 85.6989948
raytrace_times[6]["loc_std"] = 0.2444597277045035
raytrace_times[10]["loc"] = 73.6350092
raytrace_times[10]["loc_std"] = 0.3455642555169154
raytrace_times[20]["loc"] = 65.0103682
raytrace_times[20]["loc_std"] = 0.49236764128663046
sc_times[6]["loc"] = 24.6890936
sc_times[6]["loc_std"] = 0.022800330371290676
sc_times[10]["loc"] = 15.8282242
sc_times[10]["loc_std"] = 0.024118729497218546
sc_times[20]["loc"] = 14.005057
sc_times[20]["loc_std"] = 0.5506585758398755
sw_times[6]["loc"] = 9.3870636
sw_times[6]["loc_std"] = 0.03190758044477832
sw_times[10]["loc"] = 6.0323586
sw_times[10]["loc_std"] = 0.01431979964384977
sw_times[20]["loc"] = 3.6435206
sw_times[20]["loc_std"] = 0.031765626224584335
vips_times[6]["loc"] = 20.375625
vips_times[6]["loc_std"] = 0.36393135280874056
vips_times[10]["loc"] = 12.7349566
vips_times[10]["loc_std"] = 0.3293095034720984
vips_times[20]["loc"] = 7.631562
vips_times[20]["loc_std"] = 0.03708119894501795
wc_times[6]["mem"] = 10.4951606
wc_times[6]["mem_std"] = 0.11596493219521149
wc_times[10]["mem"] = 12.5078486
wc_times[10]["mem_std"] = 0.31580591793986384
wc_times[20]["mem"] = 19.1303902
wc_times[20]["mem_std"] = 0.4162827736236031
matrixmult_times[6]["mem"] = 8.0018692
matrixmult_times[6]["mem_std"] = 0.02571497402215293
matrixmult_times[10]["mem"] = 5.0255288
matrixmult_times[10]["mem_std"] = 0.017604633440091843
matrixmult_times[20]["mem"] = 3.0025516
matrixmult_times[20]["mem_std"] = 0.011293875660728694
wr_times[6]["mem"] = 17.0838738
wr_times[6]["mem_std"] = 0.22901557590295032
wr_times[10]["mem"] = 17.8708422
wr_times[10]["mem_std"] = 0.12099151268316302
wr_times[20]["mem"] = 19.5313742
wr_times[20]["mem_std"] = 0.31488246278724386
wrmem_times[6]["mem"] = 22.4487
wrmem_times[6]["mem_std"] = 0.25353855888602034
wrmem_times[10]["mem"] = 16.5552848
wrmem_times[10]["mem_std"] = 0.08607114061612058
wrmem_times[20]["mem"] = 13.2426026
wrmem_times[20]["mem_std"] = 0.12017477909544914
blackscholes_times[6]["mem"] = 4.7426438
blackscholes_times[6]["mem_std"] = 0.009951589328343488
blackscholes_times[10]["mem"] = 3.8543632
blackscholes_times[10]["mem_std"] = 0.014329749521886278
blackscholes_times[20]["mem"] = 3.1912546
blackscholes_times[20]["mem_std"] = 0.04439202143899284
bodytrack_times[6]["mem"] = 48.3730934
bodytrack_times[6]["mem_std"] = 0.5174424652869535
bodytrack_times[10]["mem"] = 36.0262574
bodytrack_times[10]["mem_std"] = 0.804484160288467
bodytrack_times[20]["mem"] = 25.3760446
bodytrack_times[20]["mem_std"] = 1.4889224026087593
canneal_times[6]["mem"] = 41.6291266
canneal_times[6]["mem_std"] = 0.4674569117720263
canneal_times[10]["mem"] = 26.84275
canneal_times[10]["mem_std"] = 0.05039263305285804
canneal_times[20]["mem"] = 15.4635736
canneal_times[20]["mem_std"] = 0.04432773659279255
facesim_times[4]["mem"] = 30.3275754
facesim_times[4]["mem_std"] = 0.38552939905465056
facesim_times[8]["mem"] = 22.3555284
facesim_times[8]["mem_std"] = 0.3555065283657109
facesim_times[16]["mem"] = 18.5029234
facesim_times[16]["mem_std"] = 0.02457541900029377
ferret_times[6]["mem"] = 69.995119
ferret_times[6]["mem_std"] = 0.1762643644427313
ferret_times[10]["mem"] = 35.587667
ferret_times[10]["mem_std"] = 0.08055759260305635
ferret_times[22]["mem"] = 15.6759846
ferret_times[22]["mem_std"] = 0.13694405436177212
fluidanimate_times[4]["mem"] = 34.7654426
fluidanimate_times[4]["mem_std"] = 0.16675950649795052
fluidanimate_times[8]["mem"] = 21.1949072
fluidanimate_times[8]["mem_std"] = 0.2612932801902108
fluidanimate_times[16]["mem"] = 13.5505244
fluidanimate_times[16]["mem_std"] = 0.1048297685404294
raytrace_times[6]["mem"] = 86.0301778
raytrace_times[6]["mem_std"] = 0.25909084564407137
raytrace_times[10]["mem"] = 74.901095
raytrace_times[10]["mem_std"] = 0.3005910541336851
raytrace_times[20]["mem"] = 65.3118316
raytrace_times[20]["mem_std"] = 0.3546470440421575
sc_times[6]["mem"] = 30.5733628
sc_times[6]["mem_std"] = 1.3929208309059635
sc_times[10]["mem"] = 18.2872218
sc_times[10]["mem_std"] = 0.5270234565441656
sc_times[20]["mem"] = 10.2461386
sc_times[20]["mem_std"] = 0.3649717662119085
sw_times[6]["mem"] = 8.494477
sw_times[6]["mem_std"] = 0.021129408718655616
sw_times[10]["mem"] = 5.7866526
sw_times[10]["mem_std"] = 0.016025405762101628
sw_times[20]["mem"] = 3.5547848
sw_times[20]["mem_std"] = 0.020836840637678256
vips_times[6]["mem"] = 19.2044756
vips_times[6]["mem_std"] = 0.25342683099987656
vips_times[10]["mem"] = 12.8168072
vips_times[10]["mem_std"] = 0.20174805247178967
vips_times[20]["mem"] = 7.6293074
vips_times[20]["mem_std"] = 0.08425501817363759



#linux time 5 runs, wc with 20 threads was executed 20 times due to its high std
wc_times[6]["linux"] = 11.7135532
wc_times[6]["linux_std"] = 0.3927948998560954
wc_times[10]["linux"] = 9.9221932
wc_times[10]["linux_std"] = 0.34114251836990356
wc_times[20]["linux"] = 18.3363797
wc_times[20]["linux_std"] = 6.815075552413004
matrixmult_times[6]["linux"] = 8.1160458
matrixmult_times[6]["linux_std"] = 0.05853091528209686
matrixmult_times[10]["linux"] = 5.165761
matrixmult_times[10]["linux_std"] = 0.01990414275471315
matrixmult_times[20]["linux"] = 2.9165922
matrixmult_times[20]["linux_std"] = 0.11519581397325165
wr_times[6]["linux"] = 13.762074
wr_times[6]["linux_std"] = 0.9505266491060627
wr_times[10]["linux"] = 14.0348828
wr_times[10]["linux_std"] = 2.267654919152771
wr_times[20]["linux"] = 13.4395144
wr_times[20]["linux_std"] = 0.5426664621059974
wrmem_times[6]["linux"] = 24.3096602
wrmem_times[6]["linux_std"] = 0.1436512244408658
wrmem_times[10]["linux"] = 17.6809118
wrmem_times[10]["linux_std"] = 0.5114229531710128
wrmem_times[20]["linux"] = 14.8795576
wrmem_times[20]["linux_std"] = 0.5014893693663306
blackscholes_times[6]["linux"] = 4.90706
blackscholes_times[6]["linux_std"] = 0.05494385559823774
blackscholes_times[10]["linux"] = 3.9166278
blackscholes_times[10]["linux_std"] = 0.025936086940014678
blackscholes_times[20]["linux"] = 3.2048604
blackscholes_times[20]["linux_std"] = 0.10119765005690597
bodytrack_times[6]["linux"] = 48.368186
bodytrack_times[6]["linux_std"] = 0.7275013580476671
bodytrack_times[10]["linux"] = 36.1323516
bodytrack_times[10]["linux_std"] = 0.42269856049937055
bodytrack_times[20]["linux"] = 24.2250508
bodytrack_times[20]["linux_std"] = 0.21965667954460205
canneal_times[6]["linux"] = 41.6153198
canneal_times[6]["linux_std"] = 0.14025558928242396
canneal_times[10]["linux"] = 26.8943984
canneal_times[10]["linux_std"] = 0.2976066395903156
canneal_times[20]["linux"] = 15.8275234
canneal_times[20]["linux_std"] = 0.10379022790532835
facesim_times[4]["linux"] = 32.555882
facesim_times[4]["linux_std"] = 0.5159525302602944
facesim_times[8]["linux"] = 22.6147958
facesim_times[8]["linux_std"] = 0.2981068712857186
facesim_times[16]["linux"] = 21.0370778
facesim_times[16]["linux_std"] = 0.1760023911467114
ferret_times[6]["linux"] = 70.8592446
ferret_times[6]["linux_std"] = 0.4219070575876161
ferret_times[10]["linux"] = 36.903823
ferret_times[10]["linux_std"] = 0.3243341474011024
ferret_times[22]["linux"] = 15.6838678
ferret_times[22]["linux_std"] = 0.10964012028340721
fluidanimate_times[4]["linux"] = 48.0415812
fluidanimate_times[4]["linux_std"] = 1.2857375067229546
fluidanimate_times[8]["linux"] = 29.0980062
fluidanimate_times[8]["linux_std"] = 0.5042738165411328
fluidanimate_times[16]["linux"] = 18.0754184
fluidanimate_times[16]["linux_std"] = 0.28299077805158246
raytrace_times[6]["linux"] = 88.21218
raytrace_times[6]["linux_std"] = 1.3722415340262806
raytrace_times[10]["linux"] = 74.7488816
raytrace_times[10]["linux_std"] = 0.9226880373145845
raytrace_times[20]["linux"] = 65.103087
raytrace_times[20]["linux_std"] = 0.40589277834620313
sc_times[6]["linux"] = 33.0493106
sc_times[6]["linux_std"] = 0.34022538800659774
sc_times[10]["linux"] = 21.2819324
sc_times[10]["linux_std"] = 0.2068960572293247
sc_times[20]["linux"] = 15.3628134
sc_times[20]["linux_std"] = 0.41404541332061634
sw_times[6]["linux"] = 8.894175
sw_times[6]["linux_std"] = 0.07121401270255735
sw_times[10]["linux"] = 5.9204694
sw_times[10]["linux_std"] = 0.15072539229021764
sw_times[20]["linux"] = 3.4774022
sw_times[20]["linux_std"] = 0.012504862180767927
vips_times[6]["linux"] = 19.9151762
vips_times[6]["linux_std"] = 0.4072465688657917
vips_times[10]["linux"] = 13.0763066
vips_times[10]["linux_std"] = 0.3744410209309872
vips_times[20]["linux"] = 7.7316392
vips_times[20]["linux_std"] = 0.06186256230516159
pca_times[6]["linux"] = 5.1674794
pca_times[6]["linux_std"] = 0.4347028356850689
pca_times[10]["linux"] = 4.9815554
pca_times[10]["linux_std"] = 0.29732185647449466
pca_times[20]["linux"] = 5.6048914
pca_times[20]["linux_std"] = 0.5872533504035886
hist_times[6]["linux"] = 18.9385632
hist_times[6]["linux_std"] = 1.952711176018553
hist_times[10]["linux"] = 12.9392212
hist_times[10]["linux_std"] = 1.0260344689241976
hist_times[20]["linux"] = 7.142124
hist_times[20]["linux_std"] = 0.07111771417867703
kmeans_times[6]["linux"] = 25.891923
kmeans_times[6]["linux_std"] = 0.2297125127049025
kmeans_times[10]["linux"] = 16.3138058
kmeans_times[10]["linux_std"] = 0.06829586851750258
kmeans_times[20]["linux"] = 9.0609344
kmeans_times[20]["linux_std"] = 0.04021541742466439
stringmatch_times[6]["linux"] = 27.32173
stringmatch_times[6]["linux_std"] = 0.14456301087484308
stringmatch_times[10]["linux"] = 17.0854932
stringmatch_times[10]["linux_std"] = 0.05884167304351568
stringmatch_times[20]["linux"] = 9.452427
stringmatch_times[20]["linux_std"] = 0.011090246615833212
linearregression_times[6]["linux"] = 7.9912344
linearregression_times[6]["linux_std"] = 0.053109263175457444
linearregression_times[10]["linux"] = 5.2595322
linearregression_times[10]["linux_std"] = 0.05275137138463795
linearregression_times[20]["linux"] = 3.3066764
linearregression_times[20]["linux_std"] = 0.05415650583484869


wc_times[6]["random"] = 10.046343363636364
wc_times[6]["random_std"] = 0.24265303298116567
wc_times[10]["random"] = 12.391975454545454
wc_times[10]["random_std"] = 0.42688554701494397
wc_times[20]["random"] = 18.512985909090908
wc_times[20]["random_std"] = 0.5660183179787888
matrixmult_times[6]["random"] = 8.152913181818182
matrixmult_times[6]["random_std"] = 0.32677352571991125
matrixmult_times[10]["random"] = 5.203840181818181
matrixmult_times[10]["random_std"] = 0.19219419458445394
matrixmult_times[20]["random"] = 3.0049563636363636
matrixmult_times[20]["random_std"] = 0.13316201114382348
wr_times[6]["random"] = 16.575892
wr_times[6]["random_std"] = 0.8182265711136272
wr_times[10]["random"] = 17.842336272727273
wr_times[10]["random_std"] = 0.6757735784608886
wr_times[20]["random"] = 19.50078218181818
wr_times[20]["random_std"] = 0.669345708639254
wrmem_times[6]["random"] = 22.655082272727274
wrmem_times[6]["random_std"] = 0.700442472462565
wrmem_times[10]["random"] = 16.78011890909091
wrmem_times[10]["random_std"] = 0.3419430614105365
wrmem_times[20]["random"] = 12.843933363636364
wrmem_times[20]["random_std"] = 0.17327080953512822
blackscholes_times[6]["random"] = 5.185326727272727
blackscholes_times[6]["random_std"] = 0.38036128643605316
blackscholes_times[10]["random"] = 4.125766909090909
blackscholes_times[10]["random_std"] = 0.24046523398591496
blackscholes_times[20]["random"] = 3.267917909090909
blackscholes_times[20]["random_std"] = 0.023579301159566916
bodytrack_times[6]["random"] = 48.00290954545454
bodytrack_times[6]["random_std"] = 2.427605325506126
bodytrack_times[10]["random"] = 37.249970363636365
bodytrack_times[10]["random_std"] = 1.3928691609020094
bodytrack_times[20]["random"] = 25.990452363636365
bodytrack_times[20]["random_std"] = 2.212203822878578
canneal_times[6]["random"] = 41.22898818181818
canneal_times[6]["random_std"] = 1.8202630885625406
canneal_times[10]["random"] = 27.714904
canneal_times[10]["random_std"] = 1.5872799438915506
canneal_times[20]["random"] = 17.73901990909091
canneal_times[20]["random_std"] = 0.8558343036788939
facesim_times[4]["random"] = 32.43340190909091
facesim_times[4]["random_std"] = 5.083460802080114
facesim_times[8]["random"] = 22.60031472727273
facesim_times[8]["random_std"] = 1.4044419480119037
facesim_times[16]["random"] = 20.092948363636363
facesim_times[16]["random_std"] = 1.1197850022993678
ferret_times[6]["random"] = 70.22656136363636
ferret_times[6]["random_std"] = 0.6538721604410664
ferret_times[10]["random"] = 36.30379290909091
ferret_times[10]["random_std"] = 0.7869379586758996
ferret_times[22]["random"] = 16.41513781818182
ferret_times[22]["random_std"] = 0.9769420889009485
fluidanimate_times[4]["random"] = 37.571130272727274
fluidanimate_times[4]["random_std"] = 2.3900607343830136
fluidanimate_times[8]["random"] = 26.79879181818182
fluidanimate_times[8]["random_std"] = 4.908020161488574
fluidanimate_times[16]["random"] = 17.10959290909091
fluidanimate_times[16]["random_std"] = 1.805418457059972
raytrace_times[6]["random"] = 85.76967336363636
raytrace_times[6]["random_std"] = 0.9502695692462766
raytrace_times[10]["random"] = 75.13976727272727
raytrace_times[10]["random_std"] = 1.4733346925386697
raytrace_times[20]["random"] = 65.43283872727272
raytrace_times[20]["random_std"] = 1.1673202829339973
sc_times[6]["random"] = 33.15732890909091
sc_times[6]["random_std"] = 3.4589139504476742
sc_times[10]["random"] = 20.095459454545455
sc_times[10]["random_std"] = 2.3504384052061966
sc_times[20]["random"] = 12.347241454545454
sc_times[20]["random_std"] = 1.2378109818018084
sw_times[6]["random"] = 10.046645636363637
sw_times[6]["random_std"] = 1.7803563281245018
sw_times[10]["random"] = 6.340803090909091
sw_times[10]["random_std"] = 0.9645738469976785
sw_times[20]["random"] = 4.4237127272727275
sw_times[20]["random_std"] = 0.38194118077313965
vips_times[6]["random"] = 20.536540090909092
vips_times[6]["random_std"] = 1.3242890337875952
vips_times[10]["random"] = 13.281809727272726
vips_times[10]["random_std"] = 0.6850208395581302
vips_times[20]["random"] = 7.958526
vips_times[20]["random_std"] = 0.6006418803183504
pca_times[6]["random"] = 5.773093
pca_times[6]["random_std"] = 0.6105900419040288
pca_times[10]["random"] = 6.050585636363636
pca_times[10]["random_std"] = 0.8831356487753553
pca_times[20]["random"] = 8.159469
pca_times[20]["random_std"] = 0.5228842989742742
hist_times[6]["random"] = 18.313184
hist_times[6]["random_std"] = 0.9023656657270881
hist_times[10]["random"] = 12.889814272727273
hist_times[10]["random_std"] = 0.6001088787722518
hist_times[20]["random"] = 7.433361909090909
hist_times[20]["random_std"] = 0.5418128764466155
kmeans_times[6]["random"] = 26.61453390909091
kmeans_times[6]["random_std"] = 3.6479210800838446
kmeans_times[10]["random"] = 18.199039090909093
kmeans_times[10]["random_std"] = 2.3554110536014905
kmeans_times[20]["random"] = 9.710131454545454
kmeans_times[20]["random_std"] = 0.638992663607816
stringmatch_times[6]["random"] = 27.26881618181818
stringmatch_times[6]["random_std"] = 0.9208006063714158
stringmatch_times[10]["random"] = 17.85587409090909
stringmatch_times[10]["random_std"] = 1.682241343560076
stringmatch_times[20]["random"] = 10.043285909090908
stringmatch_times[20]["random_std"] = 0.6533085229817748
linearregression_times[6]["random"] = 7.855553818181818
linearregression_times[6]["random_std"] = 0.20186856026624586
linearregression_times[10]["random"] = 5.408579727272727
linearregression_times[10]["random_std"] = 0.3323761838120642
linearregression_times[20]["random"] = 3.5502789090909093
linearregression_times[20]["random_std"] = 0.22394064394000582
kyotocabinet_times[6]["random"] = 7.971731636363637
kyotocabinet_times[6]["random_std"] = 0.7391660872735724
kyotocabinet_times[10]["random"] = 14.875235454545454
kyotocabinet_times[10]["random_std"] = 0.2621801348963112
kyotocabinet_times[20]["random"] = 41.10676836363636
kyotocabinet_times[20]["random_std"] = 1.5561276139199611
upscaledb_times[6]["random"] = 10.654676545454546
upscaledb_times[6]["random_std"] = 0.5778642727016225
upscaledb_times[10]["random"] = 14.757568272727273
upscaledb_times[10]["random_std"] = 0.27168893955708406
upscaledb_times[20]["random"] = 17.321144363636364
upscaledb_times[20]["random_std"] = 0.2297712079297668






wc_times[6]["random_cores"] = 10.307645818181818
wc_times[6]["random_cores_std"] = 0.26693274728454736
wc_times[10]["random_cores"] = 11.925529272727273
wc_times[10]["random_cores_std"] = 0.35283220674914456
wc_times[20]["random_cores"] = 19.289208545454546
wc_times[20]["random_cores_std"] = 0.43378873555879993
matrixmult_times[6]["random_cores"] = 7.996559
matrixmult_times[6]["random_cores_std"] = 0.08225817178084663
matrixmult_times[10]["random_cores"] = 5.134958090909091
matrixmult_times[10]["random_cores_std"] = 0.04008993956424059
matrixmult_times[20]["random_cores"] = 2.869581181818182
matrixmult_times[20]["random_cores_std"] = 0.020021679117751708
wr_times[6]["random_cores"] = 15.973533181818182
wr_times[6]["random_cores_std"] = 1.0497752754876826
wr_times[10]["random_cores"] = 17.477661545454545
wr_times[10]["random_cores_std"] = 0.5381190200235976
wr_times[20]["random_cores"] = 20.099757
wr_times[20]["random_cores_std"] = 0.3791913106311665
wrmem_times[6]["random_cores"] = 22.34504909090909
wrmem_times[6]["random_cores_std"] = 0.3395636071423691
wrmem_times[10]["random_cores"] = 16.37278790909091
wrmem_times[10]["random_cores_std"] = 0.11949432413866096
wrmem_times[20]["random_cores"] = 12.946739909090908
wrmem_times[20]["random_cores_std"] = 0.21484488611234873
blackscholes_times[6]["random_cores"] = 4.867581
blackscholes_times[6]["random_cores_std"] = 0.08510138569751002
blackscholes_times[10]["random_cores"] = 3.851484818181818
blackscholes_times[10]["random_cores_std"] = 0.015163915977910322
blackscholes_times[20]["random_cores"] = 3.016485090909091
blackscholes_times[20]["random_cores_std"] = 0.013679059925531468
bodytrack_times[6]["random_cores"] = 47.07950781818182
bodytrack_times[6]["random_cores_std"] = 0.2808463169978194
bodytrack_times[10]["random_cores"] = 35.656518636363636
bodytrack_times[10]["random_cores_std"] = 0.6041985733902199
bodytrack_times[20]["random_cores"] = 25.961315090909093
bodytrack_times[20]["random_cores_std"] = 0.29444826640359534
canneal_times[6]["random_cores"] = 39.619904636363636
canneal_times[6]["random_cores_std"] = 2.1616813665974024
canneal_times[10]["random_cores"] = 26.520362272727272
canneal_times[10]["random_cores_std"] = 0.6874181253953204
canneal_times[20]["random_cores"] = 15.576300909090909
canneal_times[20]["random_cores_std"] = 0.15890891261660478
facesim_times[4]["random_cores"] = 29.46689209090909
facesim_times[4]["random_cores_std"] = 0.5249532782291555
facesim_times[8]["random_cores"] = 22.219076454545455
facesim_times[8]["random_cores_std"] = 0.23155112759088772
facesim_times[16]["random_cores"] = 18.564400636363636
facesim_times[16]["random_cores_std"] = 0.10980036083255226
ferret_times[6]["random_cores"] = 70.59088963636364
ferret_times[6]["random_cores_std"] = 0.9291478794840389
ferret_times[10]["random_cores"] = 35.67401354545454
ferret_times[10]["random_cores_std"] = 0.27078989362548683
ferret_times[22]["random_cores"] = 15.489979636363636
ferret_times[22]["random_cores_std"] = 0.12484107920892701
fluidanimate_times[4]["random_cores"] = 39.02158881818182
fluidanimate_times[4]["random_cores_std"] = 2.8106129141171268
fluidanimate_times[8]["random_cores"] = 22.896067363636362
fluidanimate_times[8]["random_cores_std"] = 0.7024247403574503
fluidanimate_times[16]["random_cores"] = 13.552247545454545
fluidanimate_times[16]["random_cores_std"] = 0.1381877991097383
raytrace_times[6]["random_cores"] = 86.66710645454546
raytrace_times[6]["random_cores_std"] = 0.9012768623555809
raytrace_times[10]["random_cores"] = 75.024352
raytrace_times[10]["random_cores_std"] = 0.6520985749654725
raytrace_times[20]["random_cores"] = 65.20554436363636
raytrace_times[20]["random_cores_std"] = 0.7203063648037388
sc_times[6]["random_cores"] = 31.209278
sc_times[6]["random_cores_std"] = 2.8527595083456236
sc_times[10]["random_cores"] = 19.040342636363636
sc_times[10]["random_cores_std"] = 1.4764640208092548
sc_times[20]["random_cores"] = 11.463490818181818
sc_times[20]["random_cores_std"] = 0.9276981907101447
sw_times[6]["random_cores"] = 8.865730090909091
sw_times[6]["random_cores_std"] = 0.3504041602117746
sw_times[10]["random_cores"] = 5.863836454545455
sw_times[10]["random_cores_std"] = 0.05659210457517845
sw_times[20]["random_cores"] = 3.436687181818182
sw_times[20]["random_cores_std"] = 0.035089603284936335
vips_times[6]["random_cores"] = 19.705032181818183
vips_times[6]["random_cores_std"] = 0.5785823894744524
vips_times[10]["random_cores"] = 12.821242909090909
vips_times[10]["random_cores_std"] = 0.33269892877698803
vips_times[20]["random_cores"] = 7.456996545454546
vips_times[20]["random_cores_std"] = 0.039148432081371434
pca_times[6]["random_cores"] = 5.526907545454545
pca_times[6]["random_cores_std"] = 0.2895742330232012
pca_times[10]["random_cores"] = 5.695685909090909
pca_times[10]["random_cores_std"] = 0.5652274753923824
pca_times[20]["random_cores"] = 7.402023545454545
pca_times[20]["random_cores_std"] = 0.8250014394765804
hist_times[6]["random_cores"] = 18.18750127272727
hist_times[6]["random_cores_std"] = 0.7801213683352669
hist_times[10]["random_cores"] = 11.72297409090909
hist_times[10]["random_cores_std"] = 0.11485640244271385
hist_times[20]["random_cores"] = 6.925602181818181
hist_times[20]["random_cores_std"] = 0.2694826614172281
kmeans_times[6]["random_cores"] = 25.821928545454547
kmeans_times[6]["random_cores_std"] = 0.5881454758963456
kmeans_times[10]["random_cores"] = 15.961637090909091
kmeans_times[10]["random_cores_std"] = 0.16487172157410718
kmeans_times[20]["random_cores"] = 8.537603272727273
kmeans_times[20]["random_cores_std"] = 0.0813673474559453
stringmatch_times[6]["random_cores"] = 26.72349381818182
stringmatch_times[6]["random_cores_std"] = 0.23230275012414225
stringmatch_times[10]["random_cores"] = 16.815687363636364
stringmatch_times[10]["random_cores_std"] = 0.23548295690743262
stringmatch_times[20]["random_cores"] = 9.383685636363637
stringmatch_times[20]["random_cores_std"] = 0.06955944007076738
linearregression_times[6]["random_cores"] = 7.999591727272727
linearregression_times[6]["random_cores_std"] = 0.18661974471786735
linearregression_times[10]["random_cores"] = 5.218334545454545
linearregression_times[10]["random_cores_std"] = 0.07128768212016796
linearregression_times[20]["random_cores"] = 3.115819090909091
linearregression_times[20]["random_cores_std"] = 0.03601741081112286
kyotocabinet_times[6]["random_cores"] = 7.868462909090909
kyotocabinet_times[6]["random_cores_std"] = 0.5023501379641774
kyotocabinet_times[10]["random_cores"] = 14.676685363636363
kyotocabinet_times[10]["random_cores_std"] = 0.5753668037822438
kyotocabinet_times[20]["random_cores"] = 42.47700690909091
kyotocabinet_times[20]["random_cores_std"] = 0.8840041064169796
upscaledb_times[6]["random_cores"] = 10.488457363636364
upscaledb_times[6]["random_cores_std"] = 0.4169230337796769
upscaledb_times[10]["random_cores"] = 14.684194272727273
upscaledb_times[10]["random_cores_std"] = 0.14273007742594596
upscaledb_times[20]["random_cores"] = 17.549481363636364
upscaledb_times[20]["random_cores_std"] = 0.1196794751692071



mem_times = {10: {'linux': 2, 'random': 3, 'random_cores': 4, 'mem': -1.00, 'loc': 0.00}, 20: {'linux': 2, 'random': 3, 'random_cores': 4, 'mem': -1.00, 'loc': 0.00}, 6: {'linux': 2, 'random': 3, 'random_cores': 4, 'mem': -1.00, 'loc': 0.00}}
loc_times = {10: {'linux': 2, 'random': 3, 'random_cores': 4, 'mem': 0.00, 'loc': -1.00}, 20: {'linux': 2, 'random': 3, 'random_cores': 4, 'mem': 0.00, 'loc': -1.00}, 6: {'linux': 2, 'random': 3, 'random_cores': 4, 'mem': 0.00, 'loc': -1.00}}


execution_times = {"KYOTOCABINET": kyotocabinet_times, "UPSCALEDB": upscaledb_times, "WC": wc_times, "MATRIXMULT": matrixmult_times, "WR": wr_times, "WRMEM": wrmem_times,
                   "HIST": hist_times, "PCA": pca_times, "KMEANS": kmeans_times, "STRINGMATCH": stringmatch_times, "LINEARREGRESSION": linearregression_times,
                   "BLACKSCHOLES": blackscholes_times, 
                   "BODYTRACK": bodytrack_times, "CANNEAL": canneal_times, "FACESIM": facesim_times, "FERRET": ferret_times,
                   "FLUIDANIMATE": fluidanimate_times, "RAYTRACE": raytrace_times, "SC": sc_times,
                   "SW": sw_times, "VIPS": vips_times, "LOC1": loc_times, "LOC2": loc_times, "LOC3": loc_times, "LOC4": loc_times, "LOC5": loc_times,
                   "RR1": mem_times, "RR2": mem_times, "RR3": mem_times, "RR4": mem_times, "RR5": mem_times, "RR6": mem_times}

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


for i in range(0, len(classifiers)):
    clf = classifiers[i]
    clf_name = real_classifier_names[i]
    scores = cross_val_score(clf, actual_train_data, labels, cv=20)
    print("CROSS VALIDATION SCORE (" + clf_name + "): " + str(scores.mean()) + " " + str(scores.std()))
    

    
it = iter(test_filenames)
to_print = []
for fn in it:
    fn1, fn2, fn3 = (fn, next(it), next(it))
    #print("3 files: " + fn1 + ", " + fn2 + ", " + fn3)
    data = []
    app_name = get_actual_name(fn1)
    threads_number = get_threads(fn1)
    policy = get_policy(fn1)
    data.append(app_name)
    data.append(threads_number)
    data.append(policy)

    data.append(execution_times[app_name][int(threads_number)]["loc"])
    data.append(execution_times[app_name][int(threads_number)]["mem"])
    loc_time = float(execution_times[app_name][int(threads_number)]["loc"])
    mem_time = float(execution_times[app_name][int(threads_number)]["mem"])
    loc_is_better = loc_time < mem_time

    if (loc_time == 0 or mem_time == 0):
        data.append("-199")
    else:
        if (loc_time < mem_time):
            data.append(str(1 - loc_time / mem_time))
        else:
            data.append(str(1 - mem_time / loc_time))

    data.append("1") # always >= 0% 

    
    data.append(execution_times[app_name][int(threads_number)]["linux"])
    data.append(execution_times[app_name][int(threads_number)]["random"])
    data.append(execution_times[app_name][int(threads_number)]["random_cores"])

    if (float(execution_times[app_name][int(threads_number)]["loc"]) < float(execution_times[app_name][int(threads_number)]["mem"])):
        data.append("loc")
    else:
        data.append("mem")

                    
    # get the average of 3 files
    avg = len(classifiers) * [0.0]

    confidence = 0.0

    for fil in [fn1, fn2, fn3]:
        os.system("./get_input_data.py " +  fil + "| head -n 2 > " + "/tmp/foo")
        fil = "/tmp/foo"

        test_data = np.loadtxt(fil, delimiter=',')
        if (test_data.ndim == 1):
            test_data = np.reshape(test_data, (1, -1))
        
        test_data = test_data[:, 0:(cols - 1)]
        test_data = transform(test_data, False)

        if (test_data.ndim == 1):
            test_data = test_data.reshape(1, -1)

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

            #if (clf == abc): # or clf == rfc):
                #print(fn1)
               # probabilities = clf.predict_proba(test_data)
              #  predictions = clf.predict(test_data)
                #print(probabilities)
                #print(predictions)
                #print("====")
             #   confidence = confidence + np.mean(np.choose(predictions.astype(int), probabilities.T))
                
            
    print(app_name + " " + threads_number + " " + policy) # + ": " + str(confidence / 3.))


    for k in range(0, len(avg)):
        avg[k] = avg[k] / 3.
        data.append("{0:.2f}".format(avg[k]))
        if (avg[k] >= 0.5):
            if loc_is_better:
                data.append("1")
            else:
                data.append("0")
        else:
            if loc_is_better:
                data.append("0")
            else:
                data.append("1")

    to_print.append(data)
    #print("===")
    #print(to_print)
    #print("===")
        
print(tabulate(to_print, classifier_names, tablefmt='orgtbl'))

import pandas as pd
df = pd.DataFrame(to_print, columns=classifier_names)
df.to_csv('all_results.csv', index=False)
