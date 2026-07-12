/**
 * ============================================================================
 * @file        hpc_compute.cpp
 * @brief       HPC suite orchestration – CPU optimized + optional CUDA latency compare
 *
 * @author      Muhammad Samiullah (Founder & CTO)
 * @company     Embedded AI Design Labs Pvt Ltd
 * @copyright   Copyright (c) 2026 Embedded AI Design Labs Pvt Ltd. All rights reserved.
 * ============================================================================
 */

#include "hpc_compute.hpp"
#include "hpc_internal.hpp"
#include <sstream>

namespace sdv {
namespace hpc {

bool cudaAvailable() { return cuda_bridge::available(); }

std::string deviceInfo() {
    std::ostringstream oss;
    oss << "CPU-optimized always-on";
    if (cuda_bridge::available())
        oss << " | CUDA: " << cuda_bridge::info();
    else
        oss << " | " << cuda_bridge::info();
    return oss.str();
}

static BenchResult makeCpu(const char* name, const char* variant,
                           double ms, double ops, double naive_ms, const char* note) {
    BenchResult r;
    r.usecase = name;
    r.variant = variant;
    r.device = ComputeDevice::CPU_Optimized;
    r.latency_ms = ms;
    r.throughput_mops = (ms > 0.0) ? (ops / ms / 1000.0) : 0.0;
    r.speedup_vs_naive = (ms > 0.0) ? (naive_ms / ms) : 1.0;
    r.ok = true;
    r.note = note;
    return r;
}

static void maybeCuda(std::vector<BenchResult>& out, const char* name,
                      double cuda_ms, double ops, double naive_ms) {
    if (cuda_ms < 0.0) return;
    BenchResult r;
    r.usecase = name;
    r.variant = "cuda";
    r.device = ComputeDevice::CUDA_GPU;
    r.latency_ms = cuda_ms;
    r.throughput_mops = (cuda_ms > 0.0) ? (ops / cuda_ms / 1000.0) : 0.0;
    r.speedup_vs_naive = (cuda_ms > 0.0) ? (naive_ms / cuda_ms) : 1.0;
    r.ok = true;
    r.note = cuda_bridge::info();
    out.push_back(r);
}

BenchResult benchCameraPreprocess(int width, int height, int iterations) {
    auto p = cpu::benchPreprocessPair(width, height, iterations);
    return makeCpu("camera_preprocess", "optimized", p.opt_ms, p.ops, p.naive_ms,
                   "fused scale+bias, 4-wide unroll");
}

BenchResult benchConvolution2D(int width, int height, int ksize, int iterations) {
    auto p = cpu::benchConvPair(width, height, ksize, iterations);
    return makeCpu("conv2d", "optimized", p.opt_ms, p.ops, p.naive_ms, "padded hot path");
}

BenchResult benchGEMM(int M, int N, int K, int iterations) {
    auto p = cpu::benchGemmPair(M, N, K, iterations);
    return makeCpu("gemm", "optimized", p.opt_ms, p.ops, p.naive_ms, "32x32 cache blocking");
}

BenchResult benchParallelReduction(int n, int iterations) {
    auto p = cpu::benchReducePair(n, iterations);
    return makeCpu("reduce_max", "optimized", p.opt_ms, p.ops, p.naive_ms, "4-way unroll");
}

BenchResult benchFirFilter(int samples, int taps, int iterations) {
    auto p = cpu::benchFirPair(samples, taps, iterations);
    return makeCpu("fir_filter", "optimized", p.opt_ms, p.ops, p.naive_ms, "tap unroll");
}

BenchResult benchSoftmaxBatch(int batch, int classes, int iterations) {
    auto p = cpu::benchSoftmaxPair(batch, classes, iterations);
    return makeCpu("softmax", "optimized", p.opt_ms, p.ops, p.naive_ms, "stable softmax");
}

std::vector<BenchResult> runRealtimeHpcSuite() {
    std::vector<BenchResult> out;

    // Real-time ADAS/HPC oriented sizes (kept modest for laptop/Colab CPU)
    {
        auto p = cpu::benchPreprocessPair(640, 480, 15);
        out.push_back(makeCpu("camera_preprocess", "naive", p.naive_ms, p.ops, p.naive_ms, "baseline"));
        out.push_back(makeCpu("camera_preprocess", "optimized", p.opt_ms, p.ops, p.naive_ms, "fused+unroll"));
        maybeCuda(out, "camera_preprocess",
                  cuda_bridge::preprocessMs(640, 480, 30), p.ops, p.naive_ms);
    }
    {
        auto p = cpu::benchConvPair(320, 240, 3, 4);
        out.push_back(makeCpu("conv2d_3x3", "naive", p.naive_ms, p.ops, p.naive_ms, "baseline"));
        out.push_back(makeCpu("conv2d_3x3", "optimized", p.opt_ms, p.ops, p.naive_ms, "padded"));
        maybeCuda(out, "conv2d_3x3",
                  cuda_bridge::conv2dMs(320, 240, 3, 20), p.ops, p.naive_ms);
    }
    {
        auto p = cpu::benchGemmPair(128, 128, 128, 2);
        out.push_back(makeCpu("gemm_128", "naive", p.naive_ms, p.ops, p.naive_ms, "baseline"));
        out.push_back(makeCpu("gemm_128", "optimized", p.opt_ms, p.ops, p.naive_ms, "tiled"));
        maybeCuda(out, "gemm_128",
                  cuda_bridge::gemmMs(128, 128, 128, 10), p.ops, p.naive_ms);
    }
    {
        auto p = cpu::benchReducePair(1 << 20, 20);
        out.push_back(makeCpu("reduce_max_1M", "naive", p.naive_ms, p.ops, p.naive_ms, "baseline"));
        out.push_back(makeCpu("reduce_max_1M", "optimized", p.opt_ms, p.ops, p.naive_ms, "unrolled"));
        maybeCuda(out, "reduce_max_1M",
                  cuda_bridge::reduceMs(1 << 20, 50), p.ops, p.naive_ms);
    }
    {
        auto p = cpu::benchFirPair(65536, 64, 10);
        out.push_back(makeCpu("fir_radar", "naive", p.naive_ms, p.ops, p.naive_ms, "baseline"));
        out.push_back(makeCpu("fir_radar", "optimized", p.opt_ms, p.ops, p.naive_ms, "unrolled taps"));
        maybeCuda(out, "fir_radar",
                  cuda_bridge::firMs(65536, 64, 40), p.ops, p.naive_ms);
    }
    {
        auto p = cpu::benchSoftmaxPair(64, 128, 30);
        out.push_back(makeCpu("softmax_batch", "naive", p.naive_ms, p.ops, p.naive_ms, "baseline"));
        out.push_back(makeCpu("softmax_batch", "optimized", p.opt_ms, p.ops, p.naive_ms, "stable"));
        maybeCuda(out, "softmax_batch",
                  cuda_bridge::softmaxMs(64, 128, 80), p.ops, p.naive_ms);
    }

    return out;
}

} // namespace hpc
} // namespace sdv
