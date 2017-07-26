#include <mlpack/core.hpp>
#include <mlpack/methods/softmax_regression/softmax_regression.hpp>
#include <mlpack/core/optimizers/lbfgs/lbfgs.hpp>
#include <mlpack/prereqs.hpp>

#include <memory>
#include <set>
#include <ctime>


using namespace std;

// Count the number of classes in the given labels (if numClasses == 0).
size_t CalculateNumberOfClasses(const size_t numClasses,
				const arma::Row<size_t>& trainLabels);

// Test the accuracy of the model.
template<typename Model>
void TestPredictAcc(const string& testFile,
		    const size_t numClasses,
		    const Model& model);

// Build the softmax model given the parameters.
template<typename Model>
unique_ptr<Model> TrainSoftmax(const string& trainingFile,
			       const int number_of_classes,
			       const size_t maxIterations);

int main(int argc, char** argv)
{
  using namespace mlpack;


  const string trainingFile = "data.csv";
  
  
  const size_t maxIterations = 500;
  const string predictionsFile = "output.csv";


  using SM = regression::SoftmaxRegression<>;
  unique_ptr<SM> sm = TrainSoftmax<SM>(trainingFile, 0, // with 0 it will read how many classes there are are
				       maxIterations);

  // TestPredictAcc(CLI::GetParam<string>("test_data"),
  // 		 CLI::GetParam<string>("predictions_file"),
  // 		 CLI::GetParam<string>("test_labels"),
  // 		 sm->NumClasses(), *sm);

  // if (CLI::HasParam("output_model_file"))
  //   data::Save(CLI::GetParam<string>("output_model_file"),
  // 	       "softmax_regression_model", *sm, true);

  const string testFile = "test.csv";

  // arma::mat testData;
  // data::Load(testFile, testData, true);


  using namespace std;

  
  arma::Row<size_t> predictLabels;

  double matrix[12] = {0, 1, 2.1045701055093424, 2.118930937792935, 0.7522372026049572, -0.2912261713504153, 2.2709968044829294, 2.473627563643293, -0.1911659366906925, -0.4211206911574076, 3.7241400871544177, 0.8486013931098602};

  arma::mat testData(matrix, 12, 1, true, true);
  
  sm->Predict(testData, predictLabels);
  cout << "predicted labels " << predictLabels << endl;


  
  //TestPredictAcc(testFile, 2, *sm);
}


template<typename Model>
void TestPredictAcc(const string& testFile,
		    size_t numClasses,
		    const Model& model)
{
  using namespace mlpack;

  // Get the test dataset, and get predictions.
  arma::mat testData;
  data::Load(testFile, testData, true);


  arma::Row<size_t> trainLabels;
  arma::Mat<size_t> tmpTrainLabels;

  arma::Row<size_t> testLabels = arma::conv_to< arma::Row<size_t> >::from(testData.row(testData.n_rows - 1));

  testData = testData.submat(0, 0, testData.n_rows - 2, testData.n_cols - 1);

  
  arma::Row<size_t> predictLabels;
  model.Predict(testData, predictLabels);


  if (testData.n_cols != testLabels.n_elem)
    {
      cerr << "Test data in --test_data has " << testData.n_cols
		 << " points, but labels in --test_labels have "
		 << testLabels.n_elem << " labels!" << endl;
    }

  vector<size_t> bingoLabels(numClasses, 0);
  vector<size_t> labelSize(numClasses, 0);
  for (arma::uword i = 0; i != predictLabels.n_elem; ++i)
    {
      if (predictLabels(i) == testLabels(i))
	{
	  ++bingoLabels[testLabels(i)];
	}
      ++labelSize[testLabels(i)];
    }

  size_t totalBingo = 0;
  for (size_t i = 0; i != bingoLabels.size(); ++i)
    {
      cout << "Accuracy for points with label " << i << " is "
		<< (bingoLabels[i] / static_cast<double>(labelSize[i])) << " ("
		<< bingoLabels[i] << " of " << labelSize[i] << ")." << endl;
      totalBingo += bingoLabels[i];
    }

  cout << "Total accuracy for all points is "
	    << (totalBingo) / static_cast<double>(predictLabels.n_elem) << " ("
	    << totalBingo << " of " << predictLabels.n_elem << ")." << endl;

}

