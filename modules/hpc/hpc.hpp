#pragma once
/**
 * ============================================================================
 * @file        hpc.hpp
 * @brief       HPC Architecture Module – NVIDIA CUDA / CPU realtime compute suite
 *
 * Separate folder module for High-Performance Computing in the SDV stack.
 *
 * @author      Muhammad Samiullah (Founder & CTO)
 * @company     Embedded AI Design Labs Pvt Ltd
 * @email       muhammadsami@embedailabs.com
 * @copyright   Copyright (c) 2026 Embedded AI Design Labs Pvt Ltd. All rights reserved.
 * ============================================================================
 */

#include "sdv/common.hpp"
#include "hpc_compute.hpp"
#include <vector>
#include <string>

namespace sdv {
namespace hpc {

class HpcModule : public ArchitectureModule {
public:
    HpcModule();

    bool initialize() override;
    bool start() override;
    void tick(double dt_sec) override;
    void stop() override;
    void shutdown() override;
    SignalMap exportSignals() const override;
    std::vector<std::string> capabilities() const override;

    /** Execute full realtime suite (naive vs optimized vs CUDA if present). */
    std::vector<BenchResult> runSuite();

    double bestSpeedup() const { return best_speedup_; }
    double lastAvgLatencyMs() const { return avg_latency_ms_; }
    bool usingCuda() const { return cudaAvailable(); }

private:
    std::vector<BenchResult> last_results_;
    double best_speedup_;
    double avg_latency_ms_;
    double sim_time_;
};

} // namespace hpc
} // namespace sdv
