/**
 * ============================================================================
 * @file        hpc.cpp
 * @brief       HPC module lifecycle + realtime suite logging
 *
 * @author      Muhammad Samiullah (Founder & CTO)
 * @company     Embedded AI Design Labs Pvt Ltd
 * @copyright   Copyright (c) 2026 Embedded AI Design Labs Pvt Ltd. All rights reserved.
 * ============================================================================
 */

#include "hpc.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace sdv {
namespace hpc {

HpcModule::HpcModule()
    : ArchitectureModule(ArchitectureDomain::HPC, "HpcCudaRealtimeModule", ASIL::B)
    , best_speedup_(1.0)
    , avg_latency_ms_(0.0)
    , sim_time_(0.0)
{}

bool HpcModule::initialize() {
    setStatus(SystemStatus::Initializing);
    logEvent(domain_, "HPC module init – " + deviceInfo());
    logEvent(domain_, "Use-cases: preprocess, conv2d, GEMM, reduce, FIR, softmax");
    setStatus(SystemStatus::Ready);
    return true;
}

bool HpcModule::start() {
    setStatus(SystemStatus::Active);
    return true;
}

void HpcModule::tick(double dt_sec) {
    if (status_ != SystemStatus::Active) return;
    sim_time_ += dt_sec;
}

void HpcModule::stop() { setStatus(SystemStatus::Ready); }

void HpcModule::shutdown() {
    last_results_.clear();
    setStatus(SystemStatus::Offline);
}

std::vector<BenchResult> HpcModule::runSuite() {
    logEvent(domain_, "Starting realtime HPC suite (latency / speedup)...");
    last_results_ = runRealtimeHpcSuite();

    best_speedup_ = 1.0;
    double sum = 0.0;
    int n = 0;
    for (const auto& r : last_results_) {
        if (r.variant == "naive") continue;
        best_speedup_ = std::max(best_speedup_, r.speedup_vs_naive);
        sum += r.latency_ms;
        ++n;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3)
            << r.usecase << " [" << r.variant << "] "
            << r.latency_ms << " ms  speedup×" << r.speedup_vs_naive
            << "  " << r.throughput_mops << " MOPS  (" << r.note << ")";
        logEvent(domain_, oss.str());
    }
    avg_latency_ms_ = (n > 0) ? (sum / n) : 0.0;
    logEvent(domain_, "HPC suite done – best speedup×" + std::to_string(best_speedup_)
             + " avg_lat=" + std::to_string(avg_latency_ms_) + "ms");
    return last_results_;
}

SignalMap HpcModule::exportSignals() const {
    SignalMap m;
    m["hpc.best_speedup"] = {"hpc.best_speedup", best_speedup_, "x", Timestamp::now()};
    m["hpc.avg_latency_ms"] = {"hpc.avg_latency_ms", avg_latency_ms_, "ms", Timestamp::now()};
    m["hpc.cuda"] = {"hpc.cuda", cudaAvailable() ? 1.0 : 0.0, "bool", Timestamp::now()};
    m["hpc.results"] = {"hpc.results", static_cast<double>(last_results_.size()), "count", Timestamp::now()};
    return m;
}

std::vector<std::string> HpcModule::capabilities() const {
    return {
        "HPC realtime compute suite",
        "CPU optimized kernels (tiling/unroll/fusion)",
        "NVIDIA CUDA kernels (Colab / local GPU)",
        "Camera preprocess latency reduction",
        "Conv2D / GEMM / FIR / Softmax / Reduction",
        "Naive vs Optimized vs CUDA speedup report"
    };
}

} // namespace hpc
} // namespace sdv
