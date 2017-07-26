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

classifier_names = ["Application", "#threads", "(executed with) Policy", "Time (s) loc", "Time (s) mem", "Time (s) Linux", "Time (s) Slate",
                    "loc vs mem?", 
                    "QuadraticDiscriminantAnalysis", "LogisticRegressin", "Bagging", "AdaBoost", "PassiveAggressive", "Dummy", "Ridge", "MLP", "DecisionTree",
                    "NearestCentroid", "KNeighbors", "SGD", "RandomForest", "VotingClassifier"]



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





wc_times[6]["slate"] = 9.8292352
wc_times[6]["slate_std"] = 0.07440658267760991
wc_times[10]["slate"] = 9.8189966
wc_times[10]["slate_std"] = 0.05071490722696829
wc_times[20]["slate"] = 16.5823506
wc_times[20]["slate_std"] = 0.541201420482467
matrixmult_times[6]["slate"] = 8.0389402
matrixmult_times[6]["slate_std"] = 0.027553873944692425
matrixmult_times[10]["slate"] = 5.033755
matrixmult_times[10]["slate_std"] = 0.01661323704760755
matrixmult_times[20]["slate"] = 2.982185
matrixmult_times[20]["slate_std"] = 0.02902846728988632
wr_times[6]["slate"] = 13.2477334
wr_times[6]["slate_std"] = 0.08557135723967454
wr_times[10]["slate"] = 13.5014482
wr_times[10]["slate_std"] = 0.1700316149172265
wr_times[20]["slate"] = 17.7869482
wr_times[20]["slate_std"] = 0.30459173842794884
wrmem_times[6]["slate"] = 22.2017134
wrmem_times[6]["slate_std"] = 0.08760603181425353
wrmem_times[10]["slate"] = 17.2038202
wrmem_times[10]["slate_std"] = 0.12053739202156316
wrmem_times[20]["slate"] = 13.837424
wrmem_times[20]["slate_std"] = 0.1308103839150394
blackscholes_times[6]["slate"] = 4.9427452
blackscholes_times[6]["slate_std"] = 0.02233823664840177
blackscholes_times[10]["slate"] = 3.9288994
blackscholes_times[10]["slate_std"] = 0.014000806471057302
blackscholes_times[20]["slate"] = 3.2250546
blackscholes_times[20]["slate_std"] = 0.024901542623701048
bodytrack_times[6]["slate"] = 47.9038216
bodytrack_times[6]["slate_std"] = 0.26846857499945875
bodytrack_times[10]["slate"] = 36.5498102
bodytrack_times[10]["slate_std"] = 0.1520475051836103
bodytrack_times[20]["slate"] = 26.1089224
bodytrack_times[20]["slate_std"] = 0.27248863156073133
canneal_times[6]["slate"] = 27.7698492
canneal_times[6]["slate_std"] = 0.47515955003278637
canneal_times[10]["slate"] = 18.1566138
canneal_times[10]["slate_std"] = 0.09373087214872163
canneal_times[20]["slate"] = 14.7241036
canneal_times[20]["slate_std"] = 0.13877679601230172
facesim_times[4]["slate"] = 30.1137828
facesim_times[4]["slate_std"] = 0.16483142749293897
facesim_times[8]["slate"] = 21.9746628
facesim_times[8]["slate_std"] = 0.29520379321370516
facesim_times[16]["slate"] = 18.6867024
facesim_times[16]["slate_std"] = 0.04394032201793701
ferret_times[6]["slate"] = 73.2646234
ferret_times[6]["slate_std"] = 0.05106774134656829
ferret_times[10]["slate"] = 38.5705792
ferret_times[10]["slate_std"] = 0.07149097898728203
ferret_times[22]["slate"] = 15.655029
ferret_times[22]["slate_std"] = 0.0810007277917921
fluidanimate_times[4]["slate"] = 38.7157196
fluidanimate_times[4]["slate_std"] = 0.7851255183295471
fluidanimate_times[8]["slate"] = 21.0164214
fluidanimate_times[8]["slate_std"] = 0.04489179377391819
fluidanimate_times[16]["slate"] = 13.491652
fluidanimate_times[16]["slate_std"] = 0.06599098735433498
raytrace_times[6]["slate"] = 86.4484214
raytrace_times[6]["slate_std"] = 0.7528898628045406
raytrace_times[10]["slate"] = 74.4337218
raytrace_times[10]["slate_std"] = 0.377629075727704
raytrace_times[20]["slate"] = 65.8025756
raytrace_times[20]["slate_std"] = 0.5381610211535577
sc_times[6]["slate"] = 26.6987514
sc_times[6]["slate_std"] = 0.3877561372154411
sc_times[10]["slate"] = 17.5343316
sc_times[10]["slate_std"] = 0.25919871008290146
sc_times[20]["slate"] = 10.0871104
sc_times[20]["slate_std"] = 0.30291327543546187
sw_times[6]["slate"] = 8.5056152
sw_times[6]["slate_std"] = 0.017737960315662
sw_times[10]["slate"] = 5.8487516
sw_times[10]["slate_std"] = 0.13804862117761263
sw_times[20]["slate"] = 3.571515
sw_times[20]["slate_std"] = 0.025985252548320556
vips_times[6]["slate"] = 19.040498
vips_times[6]["slate_std"] = 0.39392262589244603
vips_times[10]["slate"] = 12.5707646
vips_times[10]["slate_std"] = 0.2571600942305007
vips_times[20]["slate"] = 7.5922826
vips_times[20]["slate_std"] = 0.06244391553578299
pca_times[6]["slate"] = 5.1699742
pca_times[6]["slate_std"] = 0.08377376967619399
pca_times[10]["slate"] = 4.7565756
pca_times[10]["slate_std"] = 0.23329176536740426
pca_times[20]["slate"] = 6.3183458
pca_times[20]["slate_std"] = 0.30651614397705057
hist_times[6]["slate"] = 17.4050376
hist_times[6]["slate_std"] = 0.06428874150455895
hist_times[10]["slate"] = 11.647465
hist_times[10]["slate_std"] = 0.01710647741646421
hist_times[20]["slate"] = 7.1348414
hist_times[20]["slate_std"] = 0.23325891561661688
kmeans_times[6]["slate"] = 24.9056648
kmeans_times[6]["slate_std"] = 0.14532502989285775
kmeans_times[10]["slate"] = 16.0214062
kmeans_times[10]["slate_std"] = 0.05300723253443817
kmeans_times[20]["slate"] = 8.837748
kmeans_times[20]["slate_std"] = 0.005851562047863801
stringmatch_times[6]["slate"] = 26.4579852
stringmatch_times[6]["slate_std"] = 0.03421626895147979
stringmatch_times[10]["slate"] = 16.5298104
stringmatch_times[10]["slate_std"] = 0.028705299661212387
stringmatch_times[20]["slate"] = 9.5367426
stringmatch_times[20]["slate_std"] = 0.03517287869708705
linearregression_times[6]["slate"] = 7.7413224
linearregression_times[6]["slate_std"] = 0.01762435193248251
linearregression_times[10]["slate"] = 5.2124198
linearregression_times[10]["slate_std"] = 0.014581908961449458
linearregression_times[20]["slate"] = 3.4486742
linearregression_times[20]["slate_std"] = 0.11825338889926157



mem_times = {10: {'linux': 2, 'slate': 3, 'mem': -1.00, 'loc': 0.00}, 20: {'linux': 2, 'slate': 3, 'mem': -1.00, 'loc': 0.00}, 6: {'linux': 2, 'slate': 3, 'mem': -1.00, 'loc': 0.00}}
loc_times = {10: {'linux': 2, 'slate': 3, 'mem': 0.00, 'loc': -1.00}, 20: {'linux': 2, 'slate': 3, 'mem': 0.00, 'loc': -1.00}, 6: {'linux': 2, 'slate': 3, 'mem': 0.00, 'loc': -1.00}}


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
    data.append(execution_times[app_name][int(threads_number)]["linux"])
    data.append(execution_times[app_name][int(threads_number)]["slate"])

    if (float(execution_times[app_name][int(threads_number)]["loc"]) < float(execution_times[app_name][int(threads_number)]["mem"])):
        data.append("loc")
    else:
        data.append("mem")

                    
    # get the average of 3 files
    avg = len(classifiers) * [0.0]

    confidence = 0.0

    for fil in [fn1, fn2, fn3]:
        os.system("./get_input_data.py " +  fil + " > " + "/tmp/foo")
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

            if (clf == abc): # or clf == rfc):
                #print(fn1)
                probabilities = clf.predict_proba(test_data)
                predictions = clf.predict(test_data)
                #print(probabilities)
                #print(predictions)
                #print("====")
                confidence = confidence + np.mean(np.choose(predictions.astype(int), probabilities.T))
                
            
    print(app_name + " " + threads_number + " " + policy + ": " + str(confidence / 3.))


    for k in range(0, len(avg)):
        avg[k] = avg[k] / 3.
        data.append("{0:.2f}".format(avg[k]))

    to_print.append(data)
    #print("===")
    #print(to_print)
    #print("===")
        
print(tabulate(to_print, classifier_names, tablefmt='orgtbl'))

import pandas as pd
df = pd.DataFrame(to_print, columns=classifier_names)
df.to_csv('all_results.csv', index=False)
