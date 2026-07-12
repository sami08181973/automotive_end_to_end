#pragma once
/**
 * ============================================================================
 * @file        hpc_compute.hpp
 * @brief       HPC compute API – CUDA (when built) or optimized CPU fallbacks
 *
 * Real-time automotive HPC use-cases with latency / throughput metrics:
 *   - Camera preprocess (normalize + tiled)
 *   - 2D convolution (shared-memory style tiling)
 *   - GEMM (NN / fusion layer)
 *   - Parallel reduction (max score / NMS prep)
 *   - FIR filter (radar / ultrasonic DSP)
 *   - Softmax batch (classification head)
 *
 * @author      Muhammad Samiullah (Founder & CTO)
 * @company     Embedded AI Design Labs Pvt Ltd
 * @email       muhammadsami@embedailabs.com
 * @copyright   Copyright (c) 2026 Embedded AI Design Labs Pvt Ltd. All rights reserved.
 * ============================================================================
 */

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace sdv {
namespace hpc {

enum class ComputeDevice { CPU_Optimized, CUDA_GPU };

struct BenchResult {
    std::string usecase;
    std::string variant;      /**< "naive" | "optimized" | "cuda" */
    ComputeDevice device;
    double latency_ms;
    double throughput_mops;   /**< million ops / sec (approx) */
    double speedup_vs_naive;  /**< 1.0 if this IS the naive run */
    bool   ok;
    std::string note;
};

/** True when binary was compiled with SDV_HAS_CUDA and a device is present. */
bool cudaAvailable();

std::string deviceInfo();

/* ---- Individual kernels (host API; dispatch CPU or CUDA) ---- */

BenchResult benchCameraPreprocess(int width, int height, int iterations = 20);
BenchResult benchConvolution2D(int width, int height, int ksize = 3, int iterations = 10);
BenchResult benchGEMM(int M, int N, int K, int iterations = 5);
BenchResult benchParallelReduction(int n, int iterations = 50);
BenchResult benchFirFilter(int samples, int taps = 64, int iterations = 30);
BenchResult benchSoftmaxBatch(int batch, int classes, int iterations = 40);

/** Run full real-time HPC suite; fills naive+optimized(+cuda) comparisons. */
std::vector<BenchResult> runRealtimeHpcSuite();

} // namespace hpc
} // namespace sdv
