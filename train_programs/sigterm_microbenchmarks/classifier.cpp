#define _GNU_SOURCE
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
#include <mlpack/core/optimizers/lbfgs/lbfgs.hpp>
#include <mlpack/prereqs.hpp>

#include <memory>
#include <set>
#include <ctime>



long long int min_max[11][2] = {{0, 77436373389}, {0, 304460564}, {0, 83136354}, {0, 292501258}, {0, 84155769}, {0, 20063832}, {0, 5437316}, {0, 17101458}, {0, 374}, {2, 14}, {0, 89967054}};


using namespace std;
using namespace mlpack;
using SM = regression::SoftmaxRegression<>;
unique_ptr<SM> sm;


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

  using SRF = regression::SoftmaxRegressionFunction;

  unique_ptr<Model> sm;

  arma::mat trainData;
  arma::Row<size_t> trainLabels;
  arma::Mat<size_t> tmpTrainLabels;

  data::Load(trainingFile, trainData, true);
  trainLabels = arma::conv_to< arma::Row<size_t> >::from(trainData.row(trainData.n_rows - 1));

  trainData = trainData.submat(0, 0, trainData.n_rows - 2, trainData.n_cols - 1);

  if (trainData.n_cols != trainLabels.n_elem)
    std::cerr << "Samples of input_data should same as the size input_label." << endl;

  const size_t numClasses = CalculateNumberOfClasses(number_of_classes, trainLabels);
  cerr << "The number of classes is " << numClasses << endl;

  const bool intercept = false;

  SRF smFunction(trainData, trainLabels, numClasses, intercept, 0.001);

  
  const size_t numBasis = 5;
  optimization::L_BFGS<SRF> optimizer(smFunction, numBasis, maxIterations);
  sm.reset(new Model(optimizer));


  arma::mat probabilities;
  smFunction.GetProbabilitiesMatrix(sm->Parameters(), probabilities);

  cout << "All the probabilities" << probabilities.n_rows << " , " << probabilities.n_cols << endl;
  //  for (int i = 0; i < 2962; ++i) {
  //cout << "Elem  " << i << ": " << probabilities(0, i) << " , " << probabilities(1, i) << "=" << (probabilities(0, i) + probabilities(1, i)) << endl;
  //  }

  cout << endl << endl << endl << endl;


  return sm;
}

template<typename Model>
int classify(const Model& model, int LOC, int RR, long long data[])
{
  //cerr << "aboutt to classify ... " << endl;
  double matrix[15];
  matrix[0] = LOC; // 0 or 1
  matrix[1] = RR; // 0 or 1 

  //cout << matrix[0] << ", " << matrix[1] << ", ";
  for (int i = 2; i < 13; ++i) {
    double res = (data[i - 2] - (double) min_max[i -2][0]) / ((double) min_max[i - 2][1] - min_max[i - 2][0]);
    matrix[i] = res;
    //cout << matrix[i] << ", ";
  }

  matrix[13] = data[13];


  arma::mat testData(matrix, 14, 1, true, true);
  arma::Row<size_t> predictLabels;

  model.Predict(testData, predictLabels);

  std::string features[12] = {"LOC", "RR", "retired_uops", "loads_retired_ups", "LLC_misses", "LLC_references", 
  			"local_accesses", "remote_accesses", "inter_socket1", "inter_socket2", "context_switches", "number_of_threads"};


  // GET THE PROBABILITIES of the testData
  using namespace mlpack;
  using SRF = regression::SoftmaxRegressionFunction;

  const size_t numClasses = 2;

  const bool intercept = false;
  SRF smFunction(testData, predictLabels, numClasses, intercept, 0.001);
  
  const size_t numBasis = 5;
  const size_t maxIterations = 5000;

  optimization::L_BFGS<SRF> optimizer(smFunction, numBasis, maxIterations);

  arma::mat probabilities;
  smFunction.GetProbabilitiesMatrix(model.Parameters(), probabilities);

  for (int i = 0; i < 1; ++i) {
  cout << "(" << probabilities(0, i) << " , " << probabilities(1, i) << "=" << (probabilities(0, i) + probabilities(1, i)) << ")" << endl;
  }
  cout << endl;


  // for (int i = 0; i < 12; ++i) {
  //   cout << features[i] << " ";
  // }
  // cout << endl;

  //cout << "The PARAMeETERS are: " << model.Parameters() << endl;

  return predictLabels(0);
}


int main(int argc, char* argv[]) {
  const string trainingFile = "/localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/microbenchmark_results.csv";
  const size_t maxIterations = 5000;

  sm = TrainSoftmax<SM>(trainingFile, 0, maxIterations);


  FILE* fp = fopen(argv[1], "r");

  char line[5000];
  while (fgets(line, 5000, fp) != NULL) {
    int loc, rr, final_result;
    long long int new_values[10];
    sscanf(line, "%d %d %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %d\n", &loc, &rr,
	   &new_values[0], &new_values[1], &new_values[2], &new_values[3], &new_values[4],
	   &new_values[5], &new_values[6], &new_values[7], &new_values[8], &new_values[9],
	   &final_result);

    int classification_result = classify(*sm, loc, rr, new_values);
    
    fprintf(stderr, "classification: %d\n", classification_result);
  }
  fclose(fp);

  return 0;
}

