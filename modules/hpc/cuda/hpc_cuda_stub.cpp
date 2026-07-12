/**
 * ============================================================================
 * @file        hpc_cuda_stub.cpp
 * @brief       CUDA bridge stub when SDV_HAS_CUDA is not defined (local builds)
 * ============================================================================
 */

#include "../hpc_internal.hpp"

namespace sdv {
namespace hpc {
namespace cuda_bridge {

bool available() { return false; }
std::string info() { return "CUDA not compiled in (CPU-optimized path active)"; }
double preprocessMs(int, int, int) { return -1.0; }
double conv2dMs(int, int, int, int) { return -1.0; }
double gemmMs(int, int, int, int) { return -1.0; }
double reduceMs(int, int) { return -1.0; }
double firMs(int, int, int) { return -1.0; }
double softmaxMs(int, int, int) { return -1.0; }

} // namespace cuda_bridge
} // namespace hpc
} // namespace sdv
