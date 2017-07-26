#include <string>
#include <list>
#include <cstdio>
#include "maxent.h"

#define CONCAT

using namespace std;

void train_the_model_with_samples(ME_Model & model)
{

  //#include "input_SC.h"
  //#include "input_WC.h"

  model.use_l1_regularizer(1.0);
  model.train();
}

int main()
{
  ME_Model model;

  train_the_model_with_samples(model);
  
  // Classifying a new sample
  // #include "input_WR_figure.h"
  // #include "input_CANNEAL_figure.h"
  // #include "input_FLUIDANIMATE_figure.h"
  // #include "input_BLACKSCHOLES_figure.h"

  struct timespec *start = (struct timespec *) malloc(sizeof(struct timespec));
  clock_gettime(CLOCK_MONOTONIC, start);
  //#include "input_MATRIXMULT_figure.h"
  struct timespec *finish = (struct timespec *) malloc(sizeof(struct timespec));
  clock_gettime(CLOCK_MONOTONIC, finish);

  double elapsed = (finish->tv_sec - start->tv_sec);
  elapsed += (finish->tv_nsec - start->tv_nsec) / 1000000000.0;

  cout << elapsed << endl;
  

  // #include "input_WRMEM_figure.h"
  //#include "input_SW_figure.h"

  /*
  // You can get the probability distribution of a classification
  vector<double> vp = model.classify(s);
  for (int i = 0; i < model.num_classes(); i++) {
    cout << vp[i] << "\t" <<  model.get_class_label(i) << endl;
  }
  cout << endl;
  
  // You can save the model into a file.
  model.save_to_file("model");
  // You can, of course, load a model from a file.
  // Try replacing train_the_model_with_samples() with model.load_from_file("model").

  // If you want to see the weights of the features,
  list< pair< pair<string, string>, double > > fl;
  model.get_features(fl);
  for (list< pair< pair<string, string>, double> >::const_iterator i = fl.begin(); i != fl.end(); i++) {
    printf("%10.3f  %-10s %s\n", i->second, i->first.first.c_str(), i->first.second.c_str());
    }*/
  return 0;
}
