#ifndef ROBUSTUNWRAP_H    // To make sure you don't declare the function more than once by including the header multiple times.
#define ROBUSTUNWRAP_H
//#include "mex.h"
#include <stdio.h>
#include <string.h>
#include <vector>
#include "math.h"
//#include "getopt.h"
//#include "matrix.h"

void robustUnwrapMain(std::vector<size_t> dims, float* phaseIn, float* magIn, float* output, const int numunwrapbins);

#endif //ROBUSTUNWRAP_H
