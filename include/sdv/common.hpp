#pragma once
/**
 * ============================================================================
 * @file        common.hpp
 * @brief       Umbrella header for SDV common utilities
 *
 * @author      Muhammad Samiullah (Founder & CTO)
 * @company     Embedded AI Design Labs Pvt Ltd
 * @email       muhammadsami@embedailabs.com
 * @copyright   Copyright (c) 2026 Embedded AI Design Labs Pvt Ltd. All rights reserved.
 * ============================================================================
 */

#include "copyright.hpp"
#include "types.hpp"
#include "logger.hpp"
#include "module_base.hpp"

#include <cmath>
#include <algorithm>
#include <memory>
#include <functional>
#include <thread>
#include <chrono>

namespace sdv {

inline double clamp(double v, double lo, double hi) {
    return std::max(lo, std::min(hi, v));
}

inline double lerp(double a, double b, double t) {
    return a + (b - a) * clamp(t, 0.0, 1.0);
}

inline void sleepMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

} // namespace sdv
