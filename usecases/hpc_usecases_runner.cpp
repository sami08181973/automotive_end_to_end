/**
 * ============================================================================
 * @file        hpc_usecases_runner.cpp
 * @brief       Standalone HPC realtime use-case runner (CPU + CUDA when available)
 *
 * @author      Muhammad Samiullah (Founder & CTO)
 * @company     Embedded AI Design Labs Pvt Ltd
 * @copyright   Copyright (c) 2026 Embedded AI Design Labs Pvt Ltd. All rights reserved.
 * ============================================================================
 */

#include "sdv/common.hpp"
#include "sdv/copyright.hpp"
#include "../modules/hpc/hpc.hpp"
#include <iostream>
#include <iomanip>

using namespace sdv;
using namespace sdv::hpc;

int main() {
    using namespace sdv::legal;
    std::cout << "\n=== SDV HPC Realtime Use-Case Runner ===\n"
              << AUTHOR_NAME << " | " << COMPANY_NAME << "\n"
              << COPYRIGHT_NOTICE << "\n"
              << "Device: " << deviceInfo() << "\n\n";

    HpcModule hpc;
    if (!hpc.initialize()) return 1;
    hpc.start();

    auto results = hpc.runSuite();

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "\n" << std::setw(22) << std::left << "Usecase"
              << std::setw(12) << "Variant"
              << std::setw(12) << "Latency_ms"
              << std::setw(12) << "Speedup_x"
              << std::setw(12) << "MOPS"
              << "Note\n";
    std::cout << std::string(90, '-') << "\n";
    for (const auto& r : results) {
        std::cout << std::setw(22) << r.usecase
                  << std::setw(12) << r.variant
                  << std::setw(12) << r.latency_ms
                  << std::setw(12) << r.speedup_vs_naive
                  << std::setw(12) << r.throughput_mops
                  << r.note << "\n";
    }

    std::cout << "\nBest speedup: " << hpc.bestSpeedup() << "x\n"
              << "CUDA active:  " << (hpc.usingCuda() ? "YES" : "NO (CPU path / enable on Colab)") << "\n"
              << "Watermark: " << WATERMARK << "\n\n";

    hpc.stop();
    hpc.shutdown();
    return 0;
}
