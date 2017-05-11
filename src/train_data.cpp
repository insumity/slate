#include "train_data.h"
#include "./classifier/maxent.h"

std::string train_model(ME_Model& model) {

  #include "generated_data.h"
  for (int i = 0; i < 10; ++i) {
    means_g[i] = mean[i];
    std_g[i] = std[i];
  }
  
  //model.use_l2_regularizer(1.0);

  model.use_SGD(30);

  model.train();

  return "";
}
