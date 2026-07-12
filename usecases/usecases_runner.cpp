/**
 * ============================================================================
 * @file        usecases_runner.cpp
 * @brief       Standalone Virtual ECU / cloud hardware use-case runner
 *
 * Runs all HW backends and perception/DSP/control use-cases without the
 * full vehicle stack. Useful inside Docker/Colab/K8s job containers.
 *
 * @author      Muhammad Samiullah (Founder & CTO)
 * @company     Embedded AI Design Labs Pvt Ltd
 * @copyright   Copyright (c) 2026 Embedded AI Design Labs Pvt Ltd. All rights reserved.
 * ============================================================================
 */

#include "sdv/common.hpp"
#include "sdv/copyright.hpp"
#include "../modules/virtual_ecu/virtual_ecu.hpp"
#include <iostream>
#include <iomanip>

using namespace sdv;
using namespace sdv::vecu;

int main() {
    using namespace sdv::legal;
    std::cout << "\n=== SDV Virtual ECU Use-Case Runner ===\n"
              << AUTHOR_NAME << " | " << COMPANY_NAME << "\n"
              << COPYRIGHT_NOTICE << "\n\n";

    VirtualECU ecu;
    if (!ecu.initialize()) return 1;
    ecu.start();

    auto results = ecu.runAllUsecases();

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\nResults (" << results.size() << " jobs):\n";
    for (const auto& r : results) {
        std::cout << "  [" << (r.used_accelerator ? "ACCEL" : "HOST ") << "] "
                  << std::setw(22) << std::left << r.name
                  << "  " << std::setw(8) << r.latency_ms << " ms  "
                  << r.backend_note << "\n";
    }

    ecu.stop();
    ecu.shutdown();
    std::cout << "\nAll Virtual ECU use-cases complete.\n"
              << "Watermark: " << WATERMARK << "\n\n";
    return 0;
}
