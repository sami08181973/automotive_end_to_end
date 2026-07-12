#pragma once
/** Internal CPU bench helpers – not for application code. */
#include <string>

namespace sdv {
namespace hpc {
namespace cpu {

struct PairBench {
    double naive_ms;
    double opt_ms;
    double ops;
};

PairBench benchPreprocessPair(int w, int h, int iters);
PairBench benchConvPair(int w, int h, int ks, int iters);
PairBench benchGemmPair(int M, int N, int K, int iters);
PairBench benchReducePair(int n, int iters);
PairBench benchFirPair(int samples, int taps, int iters);
PairBench benchSoftmaxPair(int batch, int C, int iters);

} // namespace cpu

namespace cuda_bridge {
bool available();
std::string info();
double preprocessMs(int w, int h, int iters);
double conv2dMs(int w, int h, int ks, int iters);
double gemmMs(int M, int N, int K, int iters);
double reduceMs(int n, int iters);
double firMs(int samples, int taps, int iters);
double softmaxMs(int batch, int classes, int iters);
}

} // namespace hpc
} // namespace sdv
