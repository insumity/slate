#ifndef __TRAIN_DATA_H
#define __TRAIN_DATA_H
#include "./classifier/maxent.h"

extern double means_g[11], std_g[11];

std::string train_model(ME_Model& model);
#endif
