/**
 * @file security.cpp
 * @brief Security Architecture implementation
 */

#include "security.hpp"
#include <sstream>

namespace sdv {
namespace security {

SecurityModule::SecurityModule()
    : ArchitectureModule(ArchitectureDomain::Security, "SecurityArchitecture", ASIL::D)
    , boot_ok_(false)
    , iso21434_controls_(true)
    , sim_time_(0.0)
{
    hsm_ = {false, false, 0, CryptoAlgo::None};
}

bool SecurityModule::initialize() {
    setStatus(SystemStatus::Initializing);

    hsm_ = {true, true, 12, CryptoAlgo::AES256};
    rules_ = {
        {"OBD",      "Powertrain", "Diag",   GatewayAction::Log},
        {"External", "CAN_Internal", "Any",  GatewayAction::Deny},
        {"TCU",      "Cloud",      "MQTT",   GatewayAction::Allow},
        {"IVI",      "VehicleNet", "SOMEIP", GatewayAction::Allow},
        {"Unknown",  "Any",        "Any",    GatewayAction::Quarantine}
    };

    logEvent(domain_, "HSM online – AES-256 keys loaded (" + std::to_string(hsm_.key_slots_used) + " slots)");
    logEvent(domain_, "ISO/SAE 21434 cybersecurity controls enabled");
    setStatus(SystemStatus::Ready);
    return true;
}

bool SecurityModule::start() {
    setStatus(SystemStatus::Active);
    boot_ok_ = runSecureBoot();
    return boot_ok_;
}

void SecurityModule::tick(double dt_sec) {
    if (status_ != SystemStatus::Active) return;
    sim_time_ += dt_sec;

    // Periodic IDS check simulation
    if (static_cast<int>(sim_time_) > 0 && static_cast<int>(sim_time_) % 5 == 0) {
        // benign
    }
}

void SecurityModule::stop() {
    setStatus(SystemStatus::Ready);
}

void SecurityModule::shutdown() {
    hsm_.available = false;
    boot_ok_ = false;
    setStatus(SystemStatus::Offline);
}

bool SecurityModule::runSecureBoot() {
    boot_chain_ = {
        {BootStage::ROM,        true, true, "ROM_RootKey"},
        {BootStage::Bootloader, true, true, "Bootloader_v2.1"},
        {BootStage::Kernel,     true, true, "SafeRTOS_Kernel"},
        {BootStage::Apps,       true, true, "VehicleApps"},
        {BootStage::Verified,   true, true, "ChainOfTrust"}
    };

    for (const auto& stage : boot_chain_) {
        if (!stage.signature_ok || !stage.hash_ok) {
            logError(domain_, "Secure boot FAILED at " + stage.image_name);
            boot_ok_ = false;
            reportThreat(ThreatLevel::Critical, "Secure boot chain broken: " + stage.image_name);
            return false;
        }
        logEvent(domain_, "Secure boot OK: " + stage.image_name);
    }
    boot_ok_ = true;
    logEvent(domain_, "Secure boot chain of trust verified");
    return true;
}

bool SecurityModule::verifyOTAPackage(const std::string& package_id, const std::string& signature) {
    if (!hsm_.available) {
        logError(domain_, "HSM unavailable – cannot verify OTA");
        return false;
    }
    // Simulated signature check
    bool ok = !signature.empty() && signature.size() >= 8;
    if (ok) {
        logEvent(domain_, "OTA package verified: " + package_id + " (sig OK)");
    } else {
        reportThreat(ThreatLevel::High, "OTA signature invalid: " + package_id);
    }
    return ok;
}

GatewayAction SecurityModule::filterMessage(const std::string& source,
                                            const std::string& dest,
                                            const std::string& service) {
    for (const auto& r : rules_) {
        bool src_match = (r.source == source || r.source == "Any" || r.source == "Unknown");
        bool dst_match = (r.dest == dest || r.dest == "Any");
        bool svc_match = (r.service == service || r.service == "Any");

        if (r.source == source && dst_match && svc_match) {
            const char* acts[] = {"Allow", "Deny", "Log", "Quarantine"};
            logEvent(domain_, std::string("Gateway ") + acts[static_cast<int>(r.action)]
                     + ": " + source + "->" + dest + "/" + service);
            if (r.action == GatewayAction::Deny || r.action == GatewayAction::Quarantine) {
                reportThreat(ThreatLevel::Medium, "Blocked: " + source + "->" + dest);
            }
            return r.action;
        }
        (void)src_match;
    }
    reportThreat(ThreatLevel::Low, "No rule match – default deny: " + source);
    return GatewayAction::Deny;
}

void SecurityModule::reportThreat(ThreatLevel level, const std::string& desc) {
    events_.push_back({level, desc, Timestamp::now(), true});
    const char* names[] = {"None", "Low", "Medium", "High", "Critical"};
    logWarn(domain_, std::string("Threat [") + names[static_cast<int>(level)] + "] " + desc);
}

std::string SecurityModule::encryptPayload(const std::string& plaintext) {
    // Demo: not real crypto – represents HSM-backed encryption
    std::ostringstream oss;
    oss << "AES256{" << plaintext.size() << "B@" << Timestamp::now().epoch_ms << "}";
    logEvent(domain_, "Payload encrypted via HSM (" + std::to_string(plaintext.size()) + " bytes)");
    return oss.str();
}

bool SecurityModule::checkISO21434Compliance() const {
    return iso21434_controls_ && hsm_.available && boot_ok_ && !rules_.empty();
}

SignalMap SecurityModule::exportSignals() const {
    SignalMap m;
    m["sec.boot_ok"] = {"sec.boot_ok", boot_ok_ ? 1.0 : 0.0, "bool", Timestamp::now()};
    m["sec.threats"] = {"sec.threats", static_cast<double>(events_.size()), "count", Timestamp::now()};
    m["sec.hsm_keys"] = {"sec.hsm_keys", static_cast<double>(hsm_.key_slots_used), "count", Timestamp::now()};
    m["sec.iso21434"] = {"sec.iso21434", checkISO21434Compliance() ? 1.0 : 0.0, "bool", Timestamp::now()};
    return m;
}

std::vector<std::string> SecurityModule::capabilities() const {
    return {
        "Secure Boot / Chain of Trust",
        "HSM-backed Encryption",
        "Central Gateway Firewall",
        "Intrusion Detection",
        "OTA Package Authentication",
        "ISO/SAE 21434 Compliance"
    };
}

} // namespace security
} // namespace sdv
