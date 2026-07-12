#pragma once
/**
 * ============================================================================
 * @file        types.hpp
 * @brief       Common types for Automotive SDV End-to-End Architecture Demo
 *
 * @author      Muhammad Samiullah (Founder & CTO)
 * @company     Embedded AI Design Labs Pvt Ltd
 * @email       muhammadsami@embedailabs.com
 * @copyright   Copyright (c) 2026 Embedded AI Design Labs Pvt Ltd. All rights reserved.
 * ============================================================================
 */

#include <cstdint>
#include <string>
#include <chrono>
#include <vector>
#include <map>

namespace sdv {

enum class ArchitectureDomain {
    Infotainment,
    Connectivity,
    ADAS,
    BodyComfort,
    Powertrain,
    ChassisSafety,
    EEArchitecture,
    Security,
    Functional,
    SoftwarePlatform,
    VehicleData,
    VirtualECU,
    HPC
};

inline const char* domainName(ArchitectureDomain d) {
    switch (d) {
        case ArchitectureDomain::Infotainment:     return "Infotainment";
        case ArchitectureDomain::Connectivity:     return "Connectivity";
        case ArchitectureDomain::ADAS:             return "ADAS & Autonomous";
        case ArchitectureDomain::BodyComfort:      return "Body & Comfort";
        case ArchitectureDomain::Powertrain:       return "Powertrain";
        case ArchitectureDomain::ChassisSafety:    return "Chassis & Safety";
        case ArchitectureDomain::EEArchitecture:   return "E/E Architecture";
        case ArchitectureDomain::Security:         return "Security";
        case ArchitectureDomain::Functional:       return "Functional";
        case ArchitectureDomain::SoftwarePlatform: return "Software Platform";
        case ArchitectureDomain::VehicleData:      return "Vehicle Data";
        case ArchitectureDomain::VirtualECU:       return "Virtual ECU / Cloud";
        case ArchitectureDomain::HPC:              return "HPC / CUDA";
        default:                                   return "Unknown";
    }
}

enum class SystemStatus {
    Offline,
    Initializing,
    Ready,
    Active,
    Degraded,
    Fault,
    Updating
};

inline const char* statusName(SystemStatus s) {
    switch (s) {
        case SystemStatus::Offline:       return "OFFLINE";
        case SystemStatus::Initializing:  return "INITIALIZING";
        case SystemStatus::Ready:         return "READY";
        case SystemStatus::Active:        return "ACTIVE";
        case SystemStatus::Degraded:      return "DEGRADED";
        case SystemStatus::Fault:         return "FAULT";
        case SystemStatus::Updating:      return "UPDATING";
        default:                          return "UNKNOWN";
    }
}

enum class ASIL {
    QM = 0,
    A  = 1,
    B  = 2,
    C  = 3,
    D  = 4
};

inline const char* asilName(ASIL a) {
    switch (a) {
        case ASIL::QM: return "ASIL-QM";
        case ASIL::A:  return "ASIL-A";
        case ASIL::B:  return "ASIL-B";
        case ASIL::C:  return "ASIL-C";
        case ASIL::D:  return "ASIL-D";
        default:       return "ASIL-?";
    }
}

struct Timestamp {
    std::int64_t epoch_ms;

    static Timestamp now() {
        using namespace std::chrono;
        auto ms = duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()).count();
        return Timestamp{ms};
    }
};

struct VehicleSignal {
    std::string name;
    double      value;
    std::string unit;
    Timestamp   ts;
};

using SignalMap = std::map<std::string, VehicleSignal>;

} // namespace sdv
