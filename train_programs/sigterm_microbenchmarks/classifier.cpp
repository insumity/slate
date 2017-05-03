#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include <mctop.h>
#include <mctop_alloc.h>
#include <sys/wait.h>
#include <time.h>
#include <numaif.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include <math.h>
#include <numaif.h>
#include <linux/hw_breakpoint.h>

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <mlpack/core.hpp>
#include <mlpack/methods/softmax_regression/softmax_regression.hpp>
#include <mlpack/methods/logistic_regression/logistic_regression.hpp>
#include <mlpack/core/optimizers/lbfgs/lbfgs.hpp>
#include <mlpack/prereqs.hpp>
#include <mlpack/core/optimizers/sgd/sgd.hpp>
#include <mlpack/core/optimizers/minibatch_sgd/minibatch_sgd.hpp>



#include <memory>
#include <set>
#include <ctime>


using namespace std;
using namespace mlpack;
using namespace mlpack::regression;
using namespace mlpack::optimization;

LogisticRegression<> sm(0, 0);

typedef struct {
  int LOC, RR;
  long long l3_hit, l3_miss, local_dram, remote_dram, l2_miss, uops_retired, unhalted_cycles, remote_fwd, remote_hitm;
  long long context_switches;
  double sockets_bw[4];
  int number_of_threads;
} my_features;

typedef struct {
  my_features feat;

  double llc_miss_rate, llc_miss_rate_square;
  double local_memory_rate, local_memory_rate_square;
  double intra_socket, intra_socket_square;
  double inter_socket, inter_socket_square;

  double llc_miss_rate_times_local_memory_rate;
  
} extended_features;

double* normalize_features(extended_features ef)
{
  double* normalized_features = (double*) malloc(26 * sizeof(double));
  my_features f = ef.feat;

  int k = 0;
  normalized_features[k++] = f.LOC;
  normalized_features[k++] = f.RR;
  normalized_features[k++] = f.l3_hit / ((double) f.unhalted_cycles);
  normalized_features[k++] = f.l3_miss / ((double) f.unhalted_cycles);
  normalized_features[k++] = f.local_dram / ((double) f.l3_miss);
  normalized_features[k++] = f.remote_dram / ((double) f.l3_miss);
  normalized_features[k++] = f.l2_miss / ((double) f.unhalted_cycles);
  normalized_features[k++] = f.uops_retired / ((double) f.unhalted_cycles);
  normalized_features[k++] = f.unhalted_cycles / (1000. * 1000. * 1000.);
  normalized_features[k++] = f.remote_fwd / ((double) f.l3_miss);
  normalized_features[k++] = f.remote_hitm / ((double) f.l3_miss);

  normalized_features[k++] = f.context_switches;

  for (int s = 0; s < 4; ++s) {
    normalized_features[k++] = f.sockets_bw[s];
  }

  normalized_features[k++] = f.number_of_threads;
  
  normalized_features[k++] = ef.llc_miss_rate;
  normalized_features[k++] = ef.llc_miss_rate_square;
  normalized_features[k++] = ef.local_memory_rate;
  normalized_features[k++] = ef.local_memory_rate_square;
  normalized_features[k++] = ef.intra_socket / f.l2_miss; // normalize
  normalized_features[k++] = (ef.intra_socket / f.l2_miss) * (ef.intra_socket / f.l2_miss);
  normalized_features[k++] = ef.inter_socket / f.l3_miss; // normalize
  normalized_features[k++] = (ef.inter_socket / f.l3_miss) * (ef.inter_socket / f.l3_miss);
  normalized_features[k++] = ef.llc_miss_rate * ef.local_memory_rate;

  return normalized_features;
}

extended_features introduce_features(my_features f)
{
  extended_features ef;
  ef.feat = f;
  
  ef.llc_miss_rate = f.l3_miss / ((double) f.l3_hit + f.l3_miss);
  ef.llc_miss_rate_square = ef.llc_miss_rate * ef.llc_miss_rate;
  ef.local_memory_rate = f.local_dram / ((double) f.local_dram + f.remote_dram);
  ef.local_memory_rate_square = ef.local_memory_rate * ef.local_memory_rate;

  double intra_socket = f.l2_miss - (f.l3_hit + f.l3_miss);
  ef.intra_socket = intra_socket >= 0? intra_socket: 0;
  ef.intra_socket_square = ef.intra_socket * ef.intra_socket;
  ef.inter_socket = f.remote_fwd + f.remote_hitm;
  ef.inter_socket_square = ef.inter_socket * ef.inter_socket;

  ef.llc_miss_rate_times_local_memory_rate = ef.llc_miss_rate * ef.local_memory_rate;

  return ef;
}




template<typename Model>
unique_ptr<Model> TrainSoftmax(const string& trainingFile,
			       const int number_of_classes,
			       const size_t maxIterations);

size_t CalculateNumberOfClasses(const size_t numClasses,
				const arma::Row<size_t>& trainLabels)
{
  if (numClasses == 0)
    {
      const set<size_t> unique_labels(begin(trainLabels),
				      end(trainLabels));
      return unique_labels.size();
    }
  else
    {
      return numClasses;
    }
}


template<typename Model>
unique_ptr<Model> TrainSoftmax(const string& trainingFile,
			       const int number_of_classes,
			       const size_t maxIterations)
{
  using namespace mlpack;

  unique_ptr<Model> sm;

  arma::mat trainDataNoNormalization;
  arma::Row<size_t> trainLabels;
  arma::Mat<size_t> tmpTrainLabels;

  data::Load(trainingFile, trainDataNoNormalization, true);
  trainLabels = arma::conv_to< arma::Row<size_t> >::from(trainDataNoNormalization.row(trainDataNoNormalization.n_rows - 1));
  // was -2 for rows ...
  trainDataNoNormalization.submat(0, 0, trainDataNoNormalization.n_rows - 2, trainDataNoNormalization.n_cols - 1);

  cout << "bEFORE NORMALIZAING\n";
  cout << trainDataNoNormalization << endl;
  cout << "===\n";
  
  arma::mat trainData(26, trainDataNoNormalization.n_cols);
  for (size_t i = 0; i < trainDataNoNormalization.n_cols; ++i) {

    my_features f;
    f.LOC = trainDataNoNormalization(0, i);
    f.RR = trainDataNoNormalization(1, i);
    f.l3_hit = trainDataNoNormalization(2, i);
    f.l3_miss = trainDataNoNormalization(3, i);
    f.local_dram = trainDataNoNormalization(4, i);
    f.remote_dram = trainDataNoNormalization(5, i);
    f.l2_miss = trainDataNoNormalization(6, i);
    f.uops_retired = trainDataNoNormalization(7, i);
    f.unhalted_cycles = trainDataNoNormalization(8, i);
    f.remote_fwd = trainDataNoNormalization(9, i);
    f.remote_hitm = trainDataNoNormalization(10, i);

    f.context_switches = trainDataNoNormalization(11, i);
    for (int j = 0; j < 4; ++j) {
      f.sockets_bw[j] = trainDataNoNormalization(12 + j, i);
    }
    
    f.number_of_threads = trainDataNoNormalization(16, i);

    double* res = normalize_features(introduce_features(f));
    for (int j = 0; j < 26; ++j) {
      trainData(j, i) = res[j];
    }
  }
  
  cout << "TRAIN DATA" << endl;
  cout << trainData << endl;
  cout << "========\n";

  cout << "TRAIN LABELS" << endl;
  cout << trainLabels << endl;
  cout << "========\n";


  if (trainData.n_cols != trainLabels.n_elem)
    std::cerr << "Samples of input_data should same as the size input_label." << endl;

  const size_t numClasses = CalculateNumberOfClasses(number_of_classes, trainLabels);

  const bool intercept = false;

  LogisticRegressionFunction<> lrf(trainData, trainLabels, model.Parameters());
  L_BFGS<LogisticRegressionFunction<>> lbfgsOpt(lrf);
  lbfgsOpt.MaxIterations() = 5000;
  lbfgsOpt.MinGradientNorm() = 0.003
  Log::Info << "Training model with L-BFGS optimizer." << endl;

  // This will train the model.
  model.Train(lbfgsOpt);

  //SRF smFunction(trainData, trainLabels, numClasses, intercept, 0.001);

  
  // const size_t numBasis = 5;
  // optimization::L_BFGS<SRF> optimizer(smFunction, numBasis, maxIterations);
  // sm.reset(new Model(optimizer));

  // arma::mat probabilities;
  // smFunction.GetProbabilitiesMatrix(sm->Parameters(), probabilities);

  // cout << "All the probabilities" << probabilities.n_rows << " , " << probabilities.n_cols << endl;
  // for (int i = 0; i < 2; ++i) {
  //   cout << "Elem  " << i << ": " << probabilities(0, i) << " , " << probabilities(1, i) << "=" << (probabilities(0, i) + probabilities(1, i)) << endl;
  // }

  cout << endl << endl << endl << endl;


  return sm;
}

template<typename Model>
int classify(const Model& model, double* normalized_features)
{
  printf("NORMALIZED\n");
  for (int i = 0; i < 26; ++i) {
    cout << normalized_features[i] << ", ";
  }
  printf("\n");
  arma::mat testData(normalized_features, 26, 1, true, true);

  cout << "TEST DATA\n";
  cout << testData << endl;
  cout << "-----\n";
  
  arma::Row<size_t> predictLabels;
  model.Classify(testData, predictLabels);
  //cout << "The PARAMeETERS are: " << model.Parameters() << endl;

  return predictLabels(0);
}


int main(int argc, char* argv[]) {


  strint fn(argv[1]);
  const string trainingFile = "/localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/" + fn;
  const size_t maxIterations = 5000;

  sm = TrainSoftmax(trainingFile, 0, maxIterations);


  FILE* fp = fopen(argv[2], "r");

  char line[5000];
  while (fgets(line, 5000, fp) != NULL) {
    int loc, rr;
    long long int new_values[11];
    int final_result;
    double socket_values[4];


    sscanf(line, "%d, %d, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lf, %lf, %lf, %lf, %lld, %d", &loc, &rr,
	   &new_values[0], &new_values[1], &new_values[2], &new_values[3], &new_values[4],
	   &new_values[5], &new_values[6], &new_values[7], &new_values[8], &new_values[9],
	   &socket_values[0], &socket_values[1], &socket_values[2], &socket_values[3], &new_values[10],
	   &final_result);


    printf("=== > %d, %d, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lf, %lf, %lf, %lf, %lld, %d\n", loc, rr,
    	   new_values[0], new_values[1], new_values[2], new_values[3], new_values[4],
    	   new_values[5], new_values[6], new_values[7], new_values[8], new_values[9],
    	   socket_values[0], socket_values[1], socket_values[2], socket_values[3], new_values[10],
    	   final_result);

    my_features feat;
    feat.LOC = loc;
    feat.RR = rr;
    feat.l3_hit = new_values[0];
    feat.l3_miss = new_values[1];
    feat.local_dram = new_values[2];
    feat.remote_dram = new_values[3];
    feat.l2_miss = new_values[4];
    feat.uops_retired = new_values[5];
    feat.unhalted_cycles = new_values[6];
    feat.remote_fwd = new_values[7];
    feat.remote_hitm = new_values[8];
    feat.context_switches = new_values[9];

    for (int i = 0; i < 4; ++i) {
      feat.sockets_bw[i] = socket_values[i];
    }

    feat.number_of_threads = new_values[10];

    double* normalized = normalize_features(introduce_features(feat));
    int classification_result = classify(*sm, normalized);

    cout << "NORMALIZED main\n";
    for (int k = 0; k < 26; ++k) {
      cout << normalized[k] << ", ";
    }
    cout << "after NORMALIZED" << endl;
    
    fprintf(stderr, "classification: %d\n", classification_result);
  }
  fclose(fp);

  return 0;
}

