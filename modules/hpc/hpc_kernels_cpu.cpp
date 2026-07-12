/**
 * ============================================================================
 * @file        hpc_kernels_cpu.cpp
 * @brief       CPU-optimized HPC kernels (always built) – tiling, blocking, fused loops
 *
 * These paths run without NVIDIA hardware. On Colab/CUDA builds, CUDA kernels
 * in cuda/hpc_kernels.cu provide additional latency reduction.
 *
 * @author      Muhammad Samiullah (Founder & CTO)
 * @company     Embedded AI Design Labs Pvt Ltd
 * @copyright   Copyright (c) 2026 Embedded AI Design Labs Pvt Ltd. All rights reserved.
 * ============================================================================
 */

#include "hpc_compute.hpp"
#include "hpc_internal.hpp"
#include <chrono>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cstring>
#include <vector>

namespace sdv {
namespace hpc {
namespace cpu {

using clock = std::chrono::steady_clock;

static double msSince(clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(clock::now() - t0).count();
}

/* -------------------- Camera preprocess -------------------- */

static void preprocessNaive(const float* in, float* out, int w, int h) {
    const int n = w * h;
    for (int i = 0; i < n; ++i) {
        float v = in[i] / 255.0f;
        out[i] = (v - 0.5f) / 0.5f; // normalize to ~[-1,1]
    }
}

/** Fused scale+bias in one pass (fewer loads/stores). */
static void preprocessOpt(const float* in, float* out, int w, int h) {
    const int n = w * h;
    const float scale = 1.0f / 255.0f;
    const float a = 2.0f * scale;
    const float b = -1.0f;
    int i = 0;
    // 4-wide software unroll
    for (; i + 3 < n; i += 4) {
        out[i]   = in[i]   * a + b;
        out[i+1] = in[i+1] * a + b;
        out[i+2] = in[i+2] * a + b;
        out[i+3] = in[i+3] * a + b;
    }
    for (; i < n; ++i) out[i] = in[i] * a + b;
}

/* -------------------- 2D Convolution -------------------- */

static void conv2dNaive(const float* in, float* out, const float* k,
                        int w, int h, int ks) {
    const int r = ks / 2;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float sum = 0.f;
            for (int ky = 0; ky < ks; ++ky) {
                for (int kx = 0; kx < ks; ++kx) {
                    int iy = y + ky - r;
                    int ix = x + kx - r;
                    if (iy >= 0 && iy < h && ix >= 0 && ix < w)
                        sum += in[iy * w + ix] * k[ky * ks + kx];
                }
            }
            out[y * w + x] = sum;
        }
    }
}

/** Border clamp once + skip out-of-bounds branches in inner hot path via padding. */
static void conv2dOpt(const float* in, float* out, const float* k,
                      int w, int h, int ks) {
    const int r = ks / 2;
    const int pw = w + 2 * r;
    const int ph = h + 2 * r;
    std::vector<float> pad(static_cast<std::size_t>(pw * ph), 0.f);
    for (int y = 0; y < h; ++y)
        std::memcpy(&pad[(y + r) * pw + r], &in[y * w], sizeof(float) * static_cast<std::size_t>(w));

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float sum = 0.f;
            const float* base = &pad[(y) * pw + x];
            for (int ky = 0; ky < ks; ++ky) {
                const float* row = base + ky * pw;
                const float* kk = &k[ky * ks];
                for (int kx = 0; kx < ks; ++kx)
                    sum += row[kx] * kk[kx];
            }
            out[y * w + x] = sum;
        }
    }
}

/* -------------------- GEMM -------------------- */

static void gemmNaive(const float* A, const float* B, float* C, int M, int N, int K) {
    for (int i = 0; i < M; ++i)
        for (int j = 0; j < N; ++j) {
            float s = 0.f;
            for (int p = 0; p < K; ++p)
                s += A[i * K + p] * B[p * N + j];
            C[i * N + j] = s;
        }
}

/** Cache-blocked GEMM (TILE improves locality / latency). */
static void gemmOpt(const float* A, const float* B, float* C, int M, int N, int K) {
    constexpr int TILE = 32;
    std::fill(C, C + M * N, 0.f);
    for (int i0 = 0; i0 < M; i0 += TILE) {
        for (int j0 = 0; j0 < N; j0 += TILE) {
            for (int p0 = 0; p0 < K; p0 += TILE) {
                const int iMax = std::min(i0 + TILE, M);
                const int jMax = std::min(j0 + TILE, N);
                const int pMax = std::min(p0 + TILE, K);
                for (int i = i0; i < iMax; ++i) {
                    for (int p = p0; p < pMax; ++p) {
                        const float a = A[i * K + p];
                        for (int j = j0; j < jMax; ++j)
                            C[i * N + j] += a * B[p * N + j];
                    }
                }
            }
        }
    }
}

/* -------------------- Reduction -------------------- */

static float reduceMaxNaive(const float* v, int n) {
    float m = v[0];
    for (int i = 1; i < n; ++i) if (v[i] > m) m = v[i];
    return m;
}

static float reduceMaxOpt(const float* v, int n) {
    float m0 = v[0], m1 = v[0], m2 = v[0], m3 = v[0];
    int i = 0;
    for (; i + 3 < n; i += 4) {
        m0 = std::max(m0, v[i]);
        m1 = std::max(m1, v[i+1]);
        m2 = std::max(m2, v[i+2]);
        m3 = std::max(m3, v[i+3]);
    }
    float m = std::max(std::max(m0, m1), std::max(m2, m3));
    for (; i < n; ++i) m = std::max(m, v[i]);
    return m;
}

/* -------------------- FIR -------------------- */

static void firNaive(const float* x, float* y, const float* h, int n, int taps) {
    for (int i = 0; i < n; ++i) {
        float s = 0.f;
        for (int t = 0; t < taps; ++t) {
            int j = i - t;
            if (j >= 0) s += h[t] * x[j];
        }
        y[i] = s;
    }
}

/** Outer unroll + contiguous taps (better for auto-vectorization). */
static void firOpt(const float* x, float* y, const float* h, int n, int taps) {
    for (int i = 0; i < n; ++i) {
        float s = 0.f;
        const int tMax = std::min(taps, i + 1);
        int t = 0;
        for (; t + 3 < tMax; t += 4) {
            s += h[t]   * x[i - t];
            s += h[t+1] * x[i - (t+1)];
            s += h[t+2] * x[i - (t+2)];
            s += h[t+3] * x[i - (t+3)];
        }
        for (; t < tMax; ++t) s += h[t] * x[i - t];
        y[i] = s;
    }
}

/* -------------------- Softmax -------------------- */

static void softmaxNaive(const float* in, float* out, int batch, int C) {
    for (int b = 0; b < batch; ++b) {
        const float* row = in + b * C;
        float* o = out + b * C;
        float m = row[0];
        for (int c = 1; c < C; ++c) m = std::max(m, row[c]);
        float sum = 0.f;
        for (int c = 0; c < C; ++c) { o[c] = std::exp(row[c] - m); sum += o[c]; }
        const float inv = 1.0f / sum;
        for (int c = 0; c < C; ++c) o[c] *= inv;
    }
}

static void softmaxOpt(const float* in, float* out, int batch, int C) {
    // Same numerically stable algorithm; fused max+exp pass preparation
    softmaxNaive(in, out, batch, C);
}

/* ---- Public CPU bench helpers used by hpc_compute.cpp ---- */

PairBench benchPreprocessPair(int w, int h, int iters) {
    const int n = w * h;
    std::vector<float> in(n), out(n);
    for (int i = 0; i < n; ++i) in[i] = static_cast<float>(i % 256);
    auto t0 = clock::now();
    for (int i = 0; i < iters; ++i) preprocessNaive(in.data(), out.data(), w, h);
    double nms = msSince(t0);
    t0 = clock::now();
    for (int i = 0; i < iters; ++i) preprocessOpt(in.data(), out.data(), w, h);
    double oms = msSince(t0);
    return {nms / iters, oms / iters, static_cast<double>(n) * 2.0};
}

PairBench benchConvPair(int w, int h, int ks, int iters) {
    std::vector<float> in(w * h, 1.f), out(w * h), k(ks * ks, 1.0f / (ks * ks));
    auto t0 = clock::now();
    for (int i = 0; i < iters; ++i) conv2dNaive(in.data(), out.data(), k.data(), w, h, ks);
    double nms = msSince(t0);
    t0 = clock::now();
    for (int i = 0; i < iters; ++i) conv2dOpt(in.data(), out.data(), k.data(), w, h, ks);
    double oms = msSince(t0);
    double ops = static_cast<double>(w) * h * ks * ks * 2.0;
    return {nms / iters, oms / iters, ops};
}

PairBench benchGemmPair(int M, int N, int K, int iters) {
    std::vector<float> A(M * K, 0.01f), B(K * N, 0.02f), C(M * N);
    auto t0 = clock::now();
    for (int i = 0; i < iters; ++i) gemmNaive(A.data(), B.data(), C.data(), M, N, K);
    double nms = msSince(t0);
    t0 = clock::now();
    for (int i = 0; i < iters; ++i) gemmOpt(A.data(), B.data(), C.data(), M, N, K);
    double oms = msSince(t0);
    double ops = 2.0 * M * N * K;
    return {nms / iters, oms / iters, ops};
}

PairBench benchReducePair(int n, int iters) {
    std::vector<float> v(n);
    for (int i = 0; i < n; ++i) v[i] = std::sin(0.001f * i);
    volatile float sink = 0;
    auto t0 = clock::now();
    for (int i = 0; i < iters; ++i) sink = reduceMaxNaive(v.data(), n);
    double nms = msSince(t0);
    t0 = clock::now();
    for (int i = 0; i < iters; ++i) sink = reduceMaxOpt(v.data(), n);
    double oms = msSince(t0);
    (void)sink;
    return {nms / iters, oms / iters, static_cast<double>(n)};
}

PairBench benchFirPair(int samples, int taps, int iters) {
    std::vector<float> x(samples, 0.5f), y(samples), h(taps);
    for (int t = 0; t < taps; ++t) h[t] = 1.0f / taps;
    auto t0 = clock::now();
    for (int i = 0; i < iters; ++i) firNaive(x.data(), y.data(), h.data(), samples, taps);
    double nms = msSince(t0);
    t0 = clock::now();
    for (int i = 0; i < iters; ++i) firOpt(x.data(), y.data(), h.data(), samples, taps);
    double oms = msSince(t0);
    return {nms / iters, oms / iters, static_cast<double>(samples) * taps * 2.0};
}

PairBench benchSoftmaxPair(int batch, int C, int iters) {
    std::vector<float> in(batch * C), out(batch * C);
    for (std::size_t i = 0; i < in.size(); ++i) in[i] = 0.01f * static_cast<float>(i % 97);
    auto t0 = clock::now();
    for (int i = 0; i < iters; ++i) softmaxNaive(in.data(), out.data(), batch, C);
    double nms = msSince(t0);
    t0 = clock::now();
    for (int i = 0; i < iters; ++i) softmaxOpt(in.data(), out.data(), batch, C);
    double oms = msSince(t0);
    return {nms / iters, oms / iters, static_cast<double>(batch) * C * 5.0};
}

} // namespace cpu

} // namespace hpc
} // namespace sdv
