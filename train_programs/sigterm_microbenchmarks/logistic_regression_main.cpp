/**
 * @file logistic_regression_main.cpp
 * @author Ryan Curtin
 *
 * Main executable for logistic regression.
 */
#include <mlpack/core.hpp>
#include <mlpack/methods/logistic_regression/logistic_regression.hpp>

#include <mlpack/core/optimizers/sgd/sgd.hpp>
#include <mlpack/core/optimizers/minibatch_sgd/minibatch_sgd.hpp>

#define NUMBER_OF_EXTENDED_FEATURES 28

using namespace std;
using namespace mlpack;
using namespace mlpack::regression;
using namespace mlpack::optimization;

PROGRAM_INFO("L2-regularized Logistic Regression and Prediction",
    "An implementation of L2-regularized logistic regression using either the "
    "L-BFGS optimizer or SGD (stochastic gradient descent).  This solves the "
    "regression problem"
    "\n\n"
    "  y = (1 / 1 + e^-(X * b))"
    "\n\n"
    "where y takes values 0 or 1."
    "\n\n"
    "This program allows loading a logistic regression model from a file (-i) "
    "or training a logistic regression model given training data (-t), or both "
    "those things at once.  In addition, this program allows classification on "
    "a test dataset (-T) and will save the classification results to the given "
    "output file (-o).  The logistic regression model itself may be saved with "
    "a file specified using the -m option."
    "\n\n"
    "The training data given with the -t option should have class labels as its"
    " last dimension (so, if the training data is in CSV format, labels should "
    "be the last column).  Alternately, the -l (--labels_file) option may be "
    "used to specify a separate file of labels."
    "\n\n"
    "When a model is being trained, there are many options.  L2 regularization "
    "(to prevent overfitting) can be specified with the -l option, and the "
    "optimizer used to train the model can be specified with the --optimizer "
    "option.  Available options are 'sgd' (stochastic gradient descent), "
    "'lbfgs' (the L-BFGS optimizer), and 'minibatch-sgd' (minibatch stochastic "
    "gradient descent).  There are also various parameters for the optimizer; "
    "the --max_iterations parameter specifies the maximum number of allowed "
     "iterations, and the --tolerance (-e) parameter specifies the tolerance "
    "for convergence.  For the SGD and mini-batch SGD optimizers, the "
    "--step_size parameter controls the step size taken at each iteration by "
    "the optimizer.  The batch size for mini-batch SGD is controlled with the "
    "--batch_size (-b) parameter.  If the objective function for your data is "
    "oscillating between Inf and 0, the step size is probably too large.  There"
    " are more parameters for the optimizers, but the C++ interface must be "
    "used to access these."
    "\n\n"
    "For SGD, an iteration refers to a single point, and for mini-batch SGD, an"
    " iteration refers to a single batch.  So to take a single pass over the "
    "dataset with SGD, --max_iterations should be set to the number of points "
    "in the dataset."
    "\n\n"
    "Optionally, the model can be used to predict the responses for another "
    "matrix of data points, if --test_file is specified.  The --test_file "
    "option can be specified without --input_file, so long as an existing "
    "logistic regression model is given with --model_file.  The output "
    "predictions from the logistic regression model are stored in the file "
    "given with --output_predictions."
    "\n\n"
    "This implementation of logistic regression does not support the general "
    "multi-class case but instead only the two-class case.  Any responses must "
    "be either 0 or 1.");

// Training parameters.
PARAM_STRING_IN("training_file", "A file containing the training set (the "
    "matrix of predictors, X).", "t", "");
PARAM_STRING_IN("labels_file", "A file containing labels (0 or 1) for the "
    "points in the training set (y).", "l", "");

// Optimizer parameters.
PARAM_DOUBLE_IN("lambda", "L2-regularization parameter for training.", "L",
    0.0);
PARAM_STRING_IN("optimizer", "Optimizer to use for training ('lbfgs' or "
    "'sgd').", "O", "lbfgs");
PARAM_DOUBLE_IN("tolerance", "Convergence tolerance for optimizer.", "e",
    1e-10);
PARAM_INT_IN("max_iterations", "Maximum iterations for optimizer (0 indicates "
    "no limit).", "n", 10000);
PARAM_DOUBLE_IN("step_size", "Step size for SGD and mini-batch SGD optimizers.",
    "s", 0.01);
PARAM_INT_IN("batch_size", "Batch size for mini-batch SGD.", "b", 50);

// Model loading/saving.
PARAM_STRING_IN("input_model_file", "File containing existing model "
    "(parameters).", "m", "");
PARAM_STRING_OUT("output_model_file", "File to save trained logistic regression"
    " model to.", "M");

// Testing.
PARAM_STRING_IN("test_file", "File containing test dataset.", "T", "");
PARAM_STRING_OUT("output_file", "If --test_file is specified, this file is "
    "where the predictions for the test set will be saved.", "o");
PARAM_STRING_OUT("output_probabilities_file", "If --test_file is specified, "
    "this file is where the class probabilities for the test set will be "
    "saved.", "p");
PARAM_DOUBLE_IN("decision_boundary", "Decision boundary for prediction; if the "
    "logistic function for a point is less than the boundary, the class is "
    "taken to be 0; otherwise, the class is 1.", "d", 0.5);



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
  double* normalized_features = (double*) malloc(NUMBER_OF_EXTENDED_FEATURES * sizeof(double));
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
  normalized_features[k++] = ef.llc_miss_rate_times_local_memory_rate;

  normalized_features[k++] = 100000000 * (f.sockets_bw[0] + f.sockets_bw[1] + f.sockets_bw[2] + f.sockets_bw[3]);
  normalized_features[k++] = 0; //((f.local_dram + f.remote_dram) / ((double) f.l3_miss)) * ef.llc_miss_rate;

  // 28 so far
  //normalized_features[k++] = (f.uops_retired / ((double) f.unhalted_cycles)) * (f.uops_retired / ((double) f.unhalted_cycles));
  //normalized_features[k++] = (f.unhalted_cycles / (- 100.)) * (f.unhalted_cycles / (- 100.));



  

  return normalized_features;
}

extended_features introduce_features(my_features f)
{
  extended_features ef;
  ef.feat = f;
  
  ef.llc_miss_rate = (f.l3_hit + f.l3_miss > 0)? f.l3_miss / ((double) f.l3_hit + f.l3_miss): 0;
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
  CLI::ParseCommandLine(argc, argv);

  // Collect command-line options.
  const string trainingFile = CLI::GetParam<string>("training_file");
  const string labelsFile = CLI::GetParam<string>("labels_file");
  const double lambda = CLI::GetParam<double>("lambda");
  const string optimizerType = CLI::GetParam<string>("optimizer");
  const double tolerance = CLI::GetParam<double>("tolerance");
  const double stepSize = CLI::GetParam<double>("step_size");
  const size_t batchSize = (size_t) CLI::GetParam<int>("batch_size");
  const size_t maxIterations = (size_t) CLI::GetParam<int>("max_iterations");
  const string inputModelFile = CLI::GetParam<string>("input_model_file");
  const string outputModelFile = CLI::GetParam<string>("output_model_file");
  const string testFile = CLI::GetParam<string>("test_file");
  const string outputFile = CLI::GetParam<string>("output_file");
  const string outputProbabilitiesFile =
      CLI::GetParam<string>("output_probabilities_file");
  const double decisionBoundary = CLI::GetParam<double>("decision_boundary");

  // One of inputFile and modelFile must be specified.
  if (trainingFile.empty() && inputModelFile.empty())
    Log::Fatal << "One of --input_model or --training_file must be specified."
        << endl;

  // If no output file is given, the user should know that the model will not be
  // saved, but only if a model is being trained.
  if (outputModelFile.empty() && !trainingFile.empty())
    Log::Warn << "--output_model_file not given; trained model will not be "
        << "saved." << endl;

  // Tolerance needs to be positive.
  if (tolerance < 0.0)
    Log::Fatal << "Tolerance must be positive (received " << tolerance << ")."
        << endl;

  // Optimizer has to be L-BFGS or SGD.
  if (optimizerType != "lbfgs" && optimizerType != "sgd" &&
      optimizerType != "minibatch-sgd")
    Log::Fatal << "--optimizer must be 'lbfgs', 'sgd', or 'minibatch-sgd'."
        << endl;

  // Lambda must be positive.
  if (lambda < 0.0)
    Log::Fatal << "L2-regularization parameter (--lambda) must be positive ("
        << "received " << lambda << ")." << endl;

  // Decision boundary must be between 0 and 1.
  if (decisionBoundary < 0.0 || decisionBoundary > 1.0)
    Log::Fatal << "Decision boundary (--decision_boundary) must be between 0.0 "
        << "and 1.0 (received " << decisionBoundary << ")." << endl;

  if ((stepSize < 0.0) &&
      (optimizerType == "sgd" || optimizerType == "minibatch-sgd"))
    Log::Fatal << "Step size (--step_size) must be positive (received "
        << stepSize << ")." << endl;

  if (CLI::HasParam("step_size") && optimizerType == "lbfgs")
    Log::Warn << "Step size (--step_size) ignored because 'sgd' optimizer is "
        << "not being used." << endl;

  if (CLI::HasParam("batch_size") && optimizerType != "minibatch-sgd")
    Log::Warn << "Batch size (--batch_size) ignored because 'minibatch-sgd' "
        << "optimizer is not being used." << endl;

  // These are the matrices we might use.
  arma::mat regressors2;
  arma::Mat<size_t> responsesMat;
  arma::Row<size_t> responses;
  //arma::mat testSet;
  arma::Row<size_t> predictions;

  // Load data matrix.
  if (!trainingFile.empty())
    data::Load(trainingFile, regressors2, true);


  regressors2.submat(0, 0, regressors2.n_rows - 1, regressors2.n_cols - 1);
  cout << "REGRESSORS 2 afters \n";
  cout << regressors2.n_rows << " < " << regressors2.n_cols << endl;


  
  arma::mat regressors(NUMBER_OF_EXTENDED_FEATURES + 1, regressors2.n_cols);
  for (size_t i = 0; i < regressors2.n_cols; ++i) {

    my_features f;
    f.LOC = regressors2(0, i);
    f.RR = regressors2(1, i);
    f.l3_hit = regressors2(2, i);
    f.l3_miss = regressors2(3, i);
    f.local_dram = regressors2(4, i);
    f.remote_dram = regressors2(5, i);
    f.l2_miss = regressors2(6, i);
    f.uops_retired = regressors2(7, i);
    f.unhalted_cycles = regressors2(8, i);
    f.remote_fwd = regressors2(9, i);
    f.remote_hitm = regressors2(10, i);

    f.context_switches = regressors2(11, i);
    for (int j = 0; j < 4; ++j) {
      f.sockets_bw[j] = regressors2(12 + j, i);
    }
    
    f.number_of_threads = regressors2(16, i);

    double* res = normalize_features(introduce_features(f));
    for (int j = 0; j < NUMBER_OF_EXTENDED_FEATURES; ++j) {
      regressors(j, i) = res[j];
    }
    regressors(NUMBER_OF_EXTENDED_FEATURES, i) = regressors2(17, i);
  }

  // Load the model, if necessary.
  LogisticRegression<> model(0, 0); // Empty model.
  if (!inputModelFile.empty())
  {
    data::Load(inputModelFile, "logistic_regression_model", model);
  }
  else
  {
    // Set the size of the parameters vector, if necessary.
    if (labelsFile.empty())
      model.Parameters() = arma::zeros<arma::vec>(regressors.n_rows - 1);
    else
      model.Parameters() = arma::zeros<arma::vec>(regressors.n_rows);
  }

  //cout << "MODEL parameters \n";
  //cout << model.Parameters() << endl;
  //cout << "====\n";
  
  // Check if the responses are in a separate file.
  if (!trainingFile.empty() && !labelsFile.empty())
  {
    data::Load(labelsFile, responsesMat, true);
    if (responsesMat.n_cols == 1)
      responses = responsesMat.col(0).t();
    else
      responses = responsesMat.row(0);

    if (responses.n_cols != regressors.n_cols)
      Log::Fatal << "The labels (--labels_file) must have the same number of "
          << "points as the training dataset (--training_file)." << endl;
  }
  else if (!trainingFile.empty())
  {
    // The initial predictors for y, Nx1.
    responses = arma::conv_to<arma::Row<size_t>>::from(
        regressors.row(regressors.n_rows - 1));
    regressors.shed_row(regressors.n_rows - 1);
  }

  // Verify the labels.
  if (!trainingFile.empty() && max(responses) > 1)
    Log::Fatal << "The labels must be either 0 or 1, not " << max(responses)
        << "!" << endl;


  // Collect command-line options.
  cout << "lambda:  " << lambda << endl;
  cout << "optimizer type: " << optimizerType << endl;
  cout << "tolerance: " << tolerance << endl;
  cout << "stepSize: " << stepSize << endl;
  cout << "batchSize: " << batchSize << endl;
  cout << "maxIterations: " << maxIterations << endl;
  cout << "decision Boundary: " << decisionBoundary << endl;


  
  // Now, do the training.
  if (!trainingFile.empty())
  {
    LogisticRegressionFunction<> lrf(regressors, responses, model.Parameters());
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
      Log::Info << "Training model with L-BFGS optimizer." << endl;

      // This will train the model.
      
      cout << "REGRESSORS ... \n" << endl;
      cout << regressors << endl;
      cout << "RESPONSES ...\n" << endl;
      cout << responses << endl;

      model.Train(lbfgsOpt);
    }
    else if (optimizerType == "minibatch-sgd")
    {
      MiniBatchSGD<LogisticRegressionFunction<>> mbsgdOpt(lrf);
      mbsgdOpt.BatchSize() = batchSize;
      mbsgdOpt.Tolerance() = tolerance;
      mbsgdOpt.StepSize() = stepSize;
      mbsgdOpt.MaxIterations() = maxIterations;
      Log::Info << "Training model with mini-batch SGD optimizer (batch size "
          << batchSize << ")." << endl;

      model.Train(mbsgdOpt);
    }
  }

  if (!testFile.empty())
  {
    arma::mat testFile2;

    data::Load(testFile, testFile2, true);

    arma::mat testSet(NUMBER_OF_EXTENDED_FEATURES, testFile2.n_cols);
    cout << "NUMBE RO FCOULMS : " << testFile2.n_cols << endl;
    for (size_t i = 0; i < testFile2.n_cols; ++i) {

      my_features f;
      f.LOC = testFile2(0, i);
      f.RR = testFile2(1, i);
      f.l3_hit = testFile2(2, i);
      f.l3_miss = testFile2(3, i);
      f.local_dram = testFile2(4, i);
      f.remote_dram = testFile2(5, i);
      f.l2_miss = testFile2(6, i);
      f.uops_retired = testFile2(7, i);
      f.unhalted_cycles = testFile2(8, i);
      f.remote_fwd = testFile2(9, i);
      f.remote_hitm = testFile2(10, i);

      f.context_switches = testFile2(11, i);
      for (int j = 0; j < 4; ++j) {
	f.sockets_bw[j] = testFile2(12 + j, i);
      }
    
      f.number_of_threads = testFile2(16, i);

      double* res = normalize_features(introduce_features(f));
      for (int j = 0; j < NUMBER_OF_EXTENDED_FEATURES; ++j) {
	testSet(j, i) = res[j];
      }
    }
    


    
    //data::Load(testFile, testSet, true);

    // We must perform predictions on the test set.  Training (and the
    // optimizer) are irrelevant here; we'll pass in the model we have.
    if (!outputFile.empty())
    {
      Log::Info << "Predicting classes of points in '" << testFile << "'."
          << endl;


      cout << "TEST SET ... \n";
      cout << testSet << endl;
      cout << endl;

      model.Classify(testSet, predictions, decisionBoundary);

      data::Save(outputFile, predictions, false);
    }

    if (!outputProbabilitiesFile.empty())
    {
      Log::Info << "Calculating class probabilities of points in '" << testFile
          << "'." << endl;
      arma::mat probabilities;
      model.Classify(testSet, probabilities);

      data::Save(outputProbabilitiesFile, probabilities, false);
    }
  }

  if (!outputModelFile.empty())
  {
    Log::Info << "Saving model to '" << outputModelFile << "'." << endl;
    data::Save(outputModelFile, "logistic_regression_model", model, false);
  }
}
