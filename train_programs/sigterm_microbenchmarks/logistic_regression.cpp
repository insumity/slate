#include <mlpack/prereqs.hpp>
#include <mlpack/core/util/cli.hpp>
#include <mlpack/core/optimizers/sgd/sgd.hpp>
#include <mlpack/core/optimizers/minibatch_sgd/minibatch_sgd.hpp>
#include <mlpack/core.hpp>
#include <mlpack/methods/logistic_regression/logistic_regression.hpp>

using namespace std;
using namespace mlpack;
using namespace mlpack::regression;
using namespace mlpack::optimization;


double lambda = 0.0;
double tolerance = 1e-10;
int maxIterations = 10000;
double stepSize = 0.01;
int batchSize = 50;

double decisionBoundary = 0.5;

LogisticRegression<> model(0, 0); // Empty model.

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
  ef.local_memory_rate = (f.local_dram + f.remote_dram > 0)? f.local_dram / ((double) f.local_dram + f.remote_dram): 0;
  ef.local_memory_rate_square = ef.local_memory_rate * ef.local_memory_rate;

  double intra_socket = f.l2_miss - (f.l3_hit + f.l3_miss);
  ef.intra_socket = intra_socket >= 0? intra_socket: 0;
  ef.intra_socket_square = ef.intra_socket * ef.intra_socket;
  ef.inter_socket = f.remote_fwd + f.remote_hitm;
  ef.inter_socket_square = ef.inter_socket * ef.inter_socket;

  ef.llc_miss_rate_times_local_memory_rate = ef.llc_miss_rate * ef.local_memory_rate;

  return ef;
}

int main(int argc, char** argv)
{
  const string optimizerType = "lfbgs";

  arma::mat regressors;
  arma::Row<size_t> responses;
  arma::mat testSet;
  arma::Row<size_t> predictions;

  // Load data matrix.
  arma::mat otrainDataNoNormalization;
  arma::Row<size_t> trainLabels;

  const string trainingFile = "/localhome/kantonia/slate/train_programs/sigterm_microbenchmarks/demo.csv";

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
  // ====
  //regressors = std::move(CLI::GetParam<arma::mat>("training"));


  
  //responses = arma::conv_to<arma::Row<size_t>>::from(
  //						     regressors.row(regressors.n_rows - 1));
  //regressors.shed_row(regressors.n_rows - 1);
  model.Parameters() = arma::zeros<arma::vec>(trainData.n_rows + 1);
  cout << "MODEL PARAMETERS ... " << endl;
  cout << model.Parameters();
  cout << endl;

  LogisticRegressionFunction<> lrf(trainData, trainLabels, model.Parameters());
  if (optimizerType == "sgd")
    {
      SGD<LogisticRegressionFunction<>> sgdOpt(lrf);
      sgdOpt.MaxIterations() = maxIterations;
      sgdOpt.Tolerance() = tolerance;
      sgdOpt.StepSize() = stepSize;
      Log::Info << "Training model with SGD optimizer." << endl;

      // This will train the model.
      model.Train(sgdOpt);
    }
  else if (optimizerType == "lbfgs")
    {
      L_BFGS<LogisticRegressionFunction<>> lbfgsOpt(lrf);
      lbfgsOpt.MaxIterations() = maxIterations;
      lbfgsOpt.MinGradientNorm() = tolerance;

      model.Train(lbfgsOpt);
    }
  else if (optimizerType == "minibatch-sgd")
    {
      MiniBatchSGD<LogisticRegressionFunction<>> mbsgdOpt(lrf);
      mbsgdOpt.BatchSize() = batchSize;
      mbsgdOpt.Tolerance() = tolerance;
      mbsgdOpt.StepSize() = stepSize;
      mbsgdOpt.MaxIterations() = maxIterations;

      model.Train(mbsgdOpt);
    }

    // Collect command-line options.
  cout << "lambda:  " << lambda << endl;
  cout << "optimizer type: " << optimizerType << endl;
  cout << "tolerance: " << tolerance << endl;
  cout << "stepSize: " << stepSize << endl;
  cout << "batchSize: " << batchSize << endl;
  cout << "maxIterations: " << maxIterations << endl;
  cout << "decision Boundary: " << decisionBoundary << endl;



  // CHANGE testSet = std::move(CLI::GetParam<arma::mat>("test"));

  // We must perform predictions on the test set.  Training (and the
  // optimizer) are irrelevant here; we'll pass in the model we have.
  // CHANGE model.Classify(testSet, predictions, decisionBoundary);

  //arma::mat probabilities;
  //model.Classify(testSet, probabilities);


  FILE* fp = fopen(argv[1], "r");

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
    arma::mat testData(normalized, 26, 1, true, true);

    cout << "NORMALIZED main\n";
    for (int k = 0; k < 26; ++k) {
      cout << normalized[k] << ", ";
    }
    cout << "after NORMALIZED" << endl;

    cout << "TEST DATA\n";
    cout << testData << endl;
    cout << "-----\n";
  
    arma::Row<size_t> predictLabels;
    //model.Classify(testData, predictLabels, decisionBoundary);
    model.Classify(testData, predictLabels);
    //cout << "The PARAMeETERS are: " << model.Parameters() << endl;

    cout << "predicted Labels : " << predictLabels << endl;
    
    int classification_result = predictLabels(0);
    
    fprintf(stderr, "classification: %d\n", classification_result);
    cerr << predictLabels << endl;
  }
  fclose(fp);






  return 0;
}
