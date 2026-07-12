/**
 * @file functional.cpp
 * @brief Functional Architecture implementation
 */

#include "functional.hpp"

namespace sdv {
namespace functional {

FunctionalModule::FunctionalModule()
    : ArchitectureModule(ArchitectureDomain::Functional, "FunctionalArchitecture", ASIL::D)
    , sim_time_(0.0)
{}

bool FunctionalModule::initialize() {
    setStatus(SystemStatus::Initializing);

    requirements_ = {
        {"REQ-IVI-001",  "System shall provide multi-display HMI",           ASIL::B, ReqStatus::Verified},
        {"REQ-CONN-001", "System shall support OTA updates over 5G",         ASIL::B, ReqStatus::Verified},
        {"REQ-ADAS-001", "System shall detect pedestrians within 40m",       ASIL::D, ReqStatus::Verified},
        {"REQ-ADAS-002", "AEB shall engage when TTC < 1.5s",                 ASIL::D, ReqStatus::Implemented},
        {"REQ-PT-001",   "BMS shall protect cells from over-voltage",        ASIL::C, ReqStatus::Verified},
        {"REQ-CH-001",   "ABS shall prevent wheel lock under max braking",   ASIL::D, ReqStatus::Verified},
        {"REQ-SEC-001",  "Secure boot shall verify all boot stages",         ASIL::D, ReqStatus::Verified},
        {"REQ-EE-001",   "Zonal backbone shall use automotive Ethernet",     ASIL::B, ReqStatus::Approved},
        {"REQ-DAT-001",  "Vehicle data shall be anonymized before cloud",    ASIL::A, ReqStatus::Implemented},
        {"REQ-SW-001",   "Platform shall support AUTOSAR Adaptive apps",     ASIL::B, ReqStatus::Approved}
    };

    features_ = {
        {"FEAT-ACC",  "Adaptive Cruise Control",   FeatureState::Standby, ASIL::C, {"REQ-ADAS-001"}},
        {"FEAT-AEB",  "Automatic Emergency Brake", FeatureState::Standby, ASIL::D, {"REQ-ADAS-002"}},
        {"FEAT-OTA",  "Over-The-Air Update",       FeatureState::Standby, ASIL::B, {"REQ-CONN-001"}},
        {"FEAT-HVAC", "Climate Control",           FeatureState::Standby, ASIL::A, {}},
        {"FEAT-TPMS", "Tire Pressure Monitor",     FeatureState::Standby, ASIL::B, {"REQ-CH-001"}},
        {"FEAT-SB",   "Secure Boot",               FeatureState::Standby, ASIL::D, {"REQ-SEC-001"}}
    };

    safety_goals_ = {
        {"SG-01", "Avoid unintended acceleration",           ASIL::D, true, false},
        {"SG-02", "Ensure braking capability always available", ASIL::D, true, false},
        {"SG-03", "Prevent unauthorized software execution", ASIL::D, true, false},
        {"SG-04", "Maintain HV battery within safe envelope", ASIL::C, true, false}
    };

    logEvent(domain_, "Feature decomposition loaded: "
             + std::to_string(features_.size()) + " features, "
             + std::to_string(requirements_.size()) + " requirements");
    setStatus(SystemStatus::Ready);
    return true;
}

bool FunctionalModule::start() {
    setStatus(SystemStatus::Active);
    activateFeature("FEAT-ACC");
    activateFeature("FEAT-AEB");
    activateFeature("FEAT-SB");
    activateFeature("FEAT-TPMS");
    evaluateSafetyGoals();
    return true;
}

void FunctionalModule::tick(double dt_sec) {
    if (status_ != SystemStatus::Active) return;
    sim_time_ += dt_sec;
    evaluateSafetyGoals();
}

void FunctionalModule::stop() {
    for (auto& f : features_) f.state = FeatureState::Inactive;
    setStatus(SystemStatus::Ready);
}

void FunctionalModule::shutdown() {
    features_.clear();
    setStatus(SystemStatus::Offline);
}

void FunctionalModule::activateFeature(const std::string& feature_id) {
    for (auto& f : features_) {
        if (f.id == feature_id) {
            f.state = FeatureState::Active;
            logEvent(domain_, "Feature ACTIVE: " + f.name + " [" + asilName(f.asil) + "]");
            return;
        }
    }
    logWarn(domain_, "Unknown feature: " + feature_id);
}

void FunctionalModule::deactivateFeature(const std::string& feature_id) {
    for (auto& f : features_) {
        if (f.id == feature_id) {
            f.state = FeatureState::Inactive;
            logEvent(domain_, "Feature INACTIVE: " + f.name);
            return;
        }
    }
}

void FunctionalModule::raiseDTC(const std::string& code, const std::string& desc) {
    dtcs_.push_back({code, desc, true, true, Timestamp::now()});
    logWarn(domain_, "DTC raised: " + code + " – " + desc);

    // Degrade related features
    if (code.find("ADAS") != std::string::npos) {
        for (auto& f : features_) {
            if (f.id.find("AEB") != std::string::npos || f.id.find("ACC") != std::string::npos)
                f.state = FeatureState::Degraded;
        }
    }
}

void FunctionalModule::clearDTC(const std::string& code) {
    for (auto& d : dtcs_) {
        if (d.code == code) {
            d.active = false;
            logEvent(domain_, "DTC cleared: " + code);
        }
    }
}

DiagResult FunctionalModule::runDiagnostic(const std::string& feature_id) {
    for (const auto& f : features_) {
        if (f.id == feature_id) {
            DiagResult r = (f.state == FeatureState::Failed) ? DiagResult::Fail : DiagResult::Pass;
            logEvent(domain_, "Diagnostic " + feature_id + ": "
                     + (r == DiagResult::Pass ? "PASS" : "FAIL"));
            return r;
        }
    }
    return DiagResult::NotTested;
}

bool FunctionalModule::evaluateSafetyGoals() {
    bool all_ok = true;
    for (auto& sg : safety_goals_) {
        // Demo: goals OK unless critical DTCs present
        sg.violated = false;
        for (const auto& d : dtcs_) {
            if (d.active && d.code.find("CRIT") != std::string::npos) {
                sg.violated = true;
                all_ok = false;
            }
        }
    }
    return all_ok;
}

int FunctionalModule::approvedRequirements() const {
    int n = 0;
    for (const auto& r : requirements_) {
        if (r.status == ReqStatus::Approved || r.status == ReqStatus::Implemented
            || r.status == ReqStatus::Verified) ++n;
    }
    return n;
}

int FunctionalModule::activeFeatures() const {
    int n = 0;
    for (const auto& f : features_) if (f.state == FeatureState::Active) ++n;
    return n;
}

SignalMap FunctionalModule::exportSignals() const {
    SignalMap m;
    m["func.features_active"] = {"func.features_active", static_cast<double>(activeFeatures()), "count", Timestamp::now()};
    m["func.requirements"] = {"func.requirements", static_cast<double>(requirements_.size()), "count", Timestamp::now()};
    m["func.dtc_count"] = {"func.dtc_count", static_cast<double>(dtcs_.size()), "count", Timestamp::now()};
    m["func.safety_goals"] = {"func.safety_goals", static_cast<double>(safety_goals_.size()), "count", Timestamp::now()};
    return m;
}

std::vector<std::string> FunctionalModule::capabilities() const {
    return {
        "Feature Decomposition",
        "Requirements Traceability",
        "ISO 26262 Safety Goals",
        "Diagnostic Trouble Codes (DTC)",
        "On-Board Diagnostics",
        "ASIL Allocation"
    };
}

} // namespace functional
} // namespace sdv
