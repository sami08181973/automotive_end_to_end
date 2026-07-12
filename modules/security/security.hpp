#pragma once
/**
 * @file security.hpp
 * @brief Security Architecture – secure boot, encryption, gateway, ISO/SAE 21434
 */

#include "sdv/common.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace sdv {
namespace security {

enum class BootStage     { ROM, Bootloader, Kernel, Apps, Verified };
enum class CryptoAlgo    { None, AES128, AES256, RSA2048, ECC256, SHA256 };
enum class ThreatLevel   { None, Low, Medium, High, Critical };
enum class GatewayAction { Allow, Deny, Log, Quarantine };

struct SecureBootChain {
    BootStage stage;
    bool      signature_ok;
    bool      hash_ok;
    std::string image_name;
};

struct FirewallRule {
    std::string source;
    std::string dest;
    std::string service;
    GatewayAction action;
};

struct SecurityEvent {
    ThreatLevel level;
    std::string description;
    Timestamp   ts;
    bool        mitigated;
};

struct HSMStatus {
    bool   available;
    bool   keys_loaded;
    int    key_slots_used;
    CryptoAlgo primary_algo;
};

class SecurityModule : public ArchitectureModule {
public:
    SecurityModule();

    bool initialize() override;
    bool start() override;
    void tick(double dt_sec) override;
    void stop() override;
    void shutdown() override;
    SignalMap exportSignals() const override;
    std::vector<std::string> capabilities() const override;

    bool runSecureBoot();
    bool verifyOTAPackage(const std::string& package_id, const std::string& signature);
    GatewayAction filterMessage(const std::string& source, const std::string& dest,
                                const std::string& service);
    void reportThreat(ThreatLevel level, const std::string& desc);
    std::string encryptPayload(const std::string& plaintext);
    bool checkISO21434Compliance() const;

    bool bootVerified() const { return boot_ok_; }
    int threatCount() const { return static_cast<int>(events_.size()); }

private:
    bool                       boot_ok_;
    std::vector<SecureBootChain> boot_chain_;
    std::vector<FirewallRule>  rules_;
    std::vector<SecurityEvent> events_;
    HSMStatus                  hsm_;
    bool                       iso21434_controls_;
    double                     sim_time_;
};

} // namespace security
} // namespace sdv
