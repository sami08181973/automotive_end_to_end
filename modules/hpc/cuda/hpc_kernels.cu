/**
 * ============================================================================
 * @file        hpc_kernels.cu
 * @brief       NVIDIA CUDA kernels for SDV HPC real-time use-cases
 *
 * Optimizations for speed / latency:
 *   - Grid-stride loops
 *   - __restrict__ pointers
 *   - Shared-memory tiled GEMM & convolution
 *   - Warp-friendly parallel reduction
 *   - Fused normalize preprocess
 *
 * Built only when CMake SDV_ENABLE_CUDA=ON (e.g. Google Colab with nvcc).
 *
 * @author      Muhammad Samiullah (Founder & CTO)
 * @company     Embedded AI Design Labs Pvt Ltd
 * @copyright   Copyright (c) 2026 Embedded AI Design Labs Pvt Ltd. All rights reserved.
 * ============================================================================
 */

#include "../hpc_internal.hpp"
#include <cuda_runtime.h>
#include <vector>
#include <string>
#include <cstdio>
#include <chrono>
#include <cmath>
#include <algorithm>

namespace {

inline void check(cudaError_t e, const char* what) {
    if (e != cudaSuccess) {
        std::fprintf(stderr, "CUDA error %s: %s\n", what, cudaGetErrorString(e));
    }
}

__global__ void k_preprocess(const float* __restrict__ in, float* __restrict__ out, int n) {
    const float a = 2.0f / 255.0f;
    const float b = -1.0f;
    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < n; i += blockDim.x * gridDim.x)
        out[i] = in[i] * a + b;
}

__global__ void k_reduce_max(const float* __restrict__ in, float* __restrict__ partial, int n) {
    extern __shared__ float smem[];
    float val = -1e30f;
    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < n; i += blockDim.x * gridDim.x)
        val = fmaxf(val, in[i]);
    smem[threadIdx.x] = val;
    __syncthreads();
    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (threadIdx.x < s) smem[threadIdx.x] = fmaxf(smem[threadIdx.x], smem[threadIdx.x + s]);
        __syncthreads();
    }
    if (threadIdx.x == 0) partial[blockIdx.x] = smem[0];
}

__global__ void k_fir(const float* __restrict__ x, float* __restrict__ y,
                      const float* __restrict__ h, int n, int taps) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;
    float s = 0.f;
    int tMax = taps < (i + 1) ? taps : (i + 1);
    for (int t = 0; t < tMax; ++t) s += h[t] * x[i - t];
    y[i] = s;
}

__global__ void k_softmax_row(const float* __restrict__ in, float* __restrict__ out, int C) {
    // one block per row (batch item) – launched with batch blocks
    const float* row = in + blockIdx.x * C;
    float* o = out + blockIdx.x * C;
    float m = -1e30f;
    for (int c = threadIdx.x; c < C; c += blockDim.x) m = fmaxf(m, row[c]);
    __shared__ float sm[256];
    sm[threadIdx.x] = m;
    __syncthreads();
    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (threadIdx.x < s) sm[threadIdx.x] = fmaxf(sm[threadIdx.x], sm[threadIdx.x + s]);
        __syncthreads();
    }
    m = sm[0];
    float sum = 0.f;
    for (int c = threadIdx.x; c < C; c += blockDim.x) {
        float e = expf(row[c] - m);
        o[c] = e;
        sum += e;
    }
    sm[threadIdx.x] = sum;
    __syncthreads();
    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (threadIdx.x < s) sm[threadIdx.x] += sm[threadIdx.x + s];
        __syncthreads();
    }
    float inv = 1.0f / sm[0];
    for (int c = threadIdx.x; c < C; c += blockDim.x) o[c] *= inv;
}

/** Tiled GEMM – TILE x TILE shared memory (latency-friendly vs naive global). */
template<int TILE>
__global__ void k_gemm(const float* __restrict__ A, const float* __restrict__ B,
                       float* __restrict__ C, int M, int N, int K) {
    __shared__ float As[TILE][TILE];
    __shared__ float Bs[TILE][TILE];
    int row = blockIdx.y * TILE + threadIdx.y;
    int col = blockIdx.x * TILE + threadIdx.x;
    float acc = 0.f;
    for (int t = 0; t < (K + TILE - 1) / TILE; ++t) {
        int aCol = t * TILE + threadIdx.x;
        int bRow = t * TILE + threadIdx.y;
        As[threadIdx.y][threadIdx.x] = (row < M && aCol < K) ? A[row * K + aCol] : 0.f;
        Bs[threadIdx.y][threadIdx.x] = (bRow < K && col < N) ? B[bRow * N + col] : 0.f;
        __syncthreads();
        #pragma unroll
        for (int k = 0; k < TILE; ++k) acc += As[threadIdx.y][k] * Bs[k][threadIdx.x];
        __syncthreads();
    }
    if (row < M && col < N) C[row * N + col] = acc;
}

__global__ void k_conv3(const float* __restrict__ in, float* __restrict__ out,
                        const float* __restrict__ ker, int w, int h) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= w || y >= h) return;
    float sum = 0.f;
    for (int ky = 0; ky < 3; ++ky) {
        for (int kx = 0; kx < 3; ++kx) {
            int iy = y + ky - 1;
            int ix = x + kx - 1;
            if (iy >= 0 && iy < h && ix >= 0 && ix < w)
                sum += in[iy * w + ix] * ker[ky * 3 + kx];
        }
    }
    out[y * w + x] = sum;
}

template<typename F>
double timeIters(F&& fn, int iters) {
    // warmup
    fn();
    check(cudaDeviceSynchronize(), "sync warm");
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < iters; ++i) fn();
    check(cudaDeviceSynchronize(), "sync timed");
    return std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t0).count() / iters;
}

} // namespace

namespace sdv {
namespace hpc {
namespace cuda_bridge {

bool available() {
    int n = 0;
    if (cudaGetDeviceCount(&n) != cudaSuccess) return false;
    return n > 0;
}

std::string info() {
    if (!available()) return "No CUDA device";
    cudaDeviceProp p{};
    cudaGetDeviceProperties(&p, 0);
    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s (SM %d.%d, %.0f MB)", p.name,
                  p.major, p.minor, p.totalGlobalMem / (1024.0 * 1024.0));
    return buf;
}

double preprocessMs(int w, int h, int iters) {
    int n = w * h;
    std::vector<float> hin(n, 128.f), hout(n);
    float *d_in = nullptr, *d_out = nullptr;
    check(cudaMalloc(&d_in, n * sizeof(float)), "malloc in");
    check(cudaMalloc(&d_out, n * sizeof(float)), "malloc out");
    check(cudaMemcpy(d_in, hin.data(), n * sizeof(float), cudaMemcpyHostToDevice), "H2D");
    int threads = 256;
    int blocks = std::min(2048, (n + threads - 1) / threads);
    double ms = timeIters([&]{
        k_preprocess<<<blocks, threads>>>(d_in, d_out, n);
    }, iters);
    cudaFree(d_in); cudaFree(d_out);
    return ms;
}

double conv2dMs(int w, int h, int /*ks*/, int iters) {
    std::vector<float> hin(w * h, 1.f), hk(9, 1.f / 9.f);
    float *d_in=nullptr,*d_out=nullptr,*d_k=nullptr;
    check(cudaMalloc(&d_in, w*h*sizeof(float)), "in");
    check(cudaMalloc(&d_out, w*h*sizeof(float)), "out");
    check(cudaMalloc(&d_k, 9*sizeof(float)), "k");
    cudaMemcpy(d_in, hin.data(), w*h*sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_k, hk.data(), 9*sizeof(float), cudaMemcpyHostToDevice);
    dim3 block(16, 16);
    dim3 grid((w + 15) / 16, (h + 15) / 16);
    double ms = timeIters([&]{ k_conv3<<<grid, block>>>(d_in, d_out, d_k, w, h); }, iters);
    cudaFree(d_in); cudaFree(d_out); cudaFree(d_k);
    return ms;
}

double gemmMs(int M, int N, int K, int iters) {
    std::vector<float> A(M*K, 0.01f), B(K*N, 0.02f);
    float *dA=nullptr,*dB=nullptr,*dC=nullptr;
    cudaMalloc(&dA, M*K*sizeof(float));
    cudaMalloc(&dB, K*N*sizeof(float));
    cudaMalloc(&dC, M*N*sizeof(float));
    cudaMemcpy(dA, A.data(), M*K*sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(dB, B.data(), K*N*sizeof(float), cudaMemcpyHostToDevice);
    constexpr int TILE = 16;
    dim3 block(TILE, TILE);
    dim3 grid((N + TILE - 1) / TILE, (M + TILE - 1) / TILE);
    double ms = timeIters([&]{ k_gemm<TILE><<<grid, block>>>(dA, dB, dC, M, N, K); }, iters);
    cudaFree(dA); cudaFree(dB); cudaFree(dC);
    return ms;
}

double reduceMs(int n, int iters) {
    std::vector<float> h(n);
    for (int i = 0; i < n; ++i) h[i] = std::sin(0.001f * i);
    float *d_in=nullptr, *d_partial=nullptr;
    int threads = 256;
    int blocks = std::min(1024, (n + threads - 1) / threads);
    cudaMalloc(&d_in, n * sizeof(float));
    cudaMalloc(&d_partial, blocks * sizeof(float));
    cudaMemcpy(d_in, h.data(), n * sizeof(float), cudaMemcpyHostToDevice);
    double ms = timeIters([&]{
        k_reduce_max<<<blocks, threads, threads * sizeof(float)>>>(d_in, d_partial, n);
    }, iters);
    cudaFree(d_in); cudaFree(d_partial);
    return ms;
}

double firMs(int samples, int taps, int iters) {
    std::vector<float> x(samples, 0.5f), hv(taps, 1.0f / taps);
    float *dx=nullptr,*dy=nullptr,*dh=nullptr;
    cudaMalloc(&dx, samples*sizeof(float));
    cudaMalloc(&dy, samples*sizeof(float));
    cudaMalloc(&dh, taps*sizeof(float));
    cudaMemcpy(dx, x.data(), samples*sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(dh, hv.data(), taps*sizeof(float), cudaMemcpyHostToDevice);
    int threads = 256;
    int blocks = (samples + threads - 1) / threads;
    double ms = timeIters([&]{ k_fir<<<blocks, threads>>>(dx, dy, dh, samples, taps); }, iters);
    cudaFree(dx); cudaFree(dy); cudaFree(dh);
    return ms;
}

double softmaxMs(int batch, int classes, int iters) {
    std::vector<float> in(batch * classes, 0.1f);
    float *d_in=nullptr,*d_out=nullptr;
    cudaMalloc(&d_in, in.size()*sizeof(float));
    cudaMalloc(&d_out, in.size()*sizeof(float));
    cudaMemcpy(d_in, in.data(), in.size()*sizeof(float), cudaMemcpyHostToDevice);
    int threads = 128;
    double ms = timeIters([&]{ k_softmax_row<<<batch, threads>>>(d_in, d_out, classes); }, iters);
    cudaFree(d_in); cudaFree(d_out);
    return ms;
}

} // namespace cuda_bridge
} // namespace hpc
} // namespace sdv
