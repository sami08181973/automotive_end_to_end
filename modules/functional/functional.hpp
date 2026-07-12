#pragma once
/**
 * @file functional.hpp
 * @brief Functional Architecture – feature decomposition, requirements, safety, diagnostics
 */

#include "sdv/common.hpp"
#include <string>
#include <vector>

namespace sdv {
namespace functional {

enum class ReqStatus   { Draft, Approved, Implemented, Verified, Deferred };
enum class FeatureState{ Inactive, Standby, Active, Failed, Degraded };
enum class DiagResult  { Pass, Fail, NotTested, Incomplete };

struct Requirement {
    std::string id;
    std::string text;
    ASIL        asil;
    ReqStatus   status;
};

struct Feature {
    std::string  id;
    std::string  name;
    FeatureState state;
    ASIL         asil;
    std::vector<std::string> req_ids;
};

struct DiagnosticTroubleCode {
    std::string code;      // e.g. P0A80
    std::string description;
    bool        active;
    bool        confirmed;
    Timestamp   first_seen;
};

struct SafetyGoal {
    std::string id;
    std::string description;
    ASIL        asil;
    bool        monitored;
    bool        violated;
};

class FunctionalModule : public ArchitectureModule {
public:
    FunctionalModule();

    bool initialize() override;
    bool start() override;
    void tick(double dt_sec) override;
    void stop() override;
    void shutdown() override;
    SignalMap exportSignals() const override;
    std::vector<std::string> capabilities() const override;

    void activateFeature(const std::string& feature_id);
    void deactivateFeature(const std::string& feature_id);
    void raiseDTC(const std::string& code, const std::string& desc);
    void clearDTC(const std::string& code);
    DiagResult runDiagnostic(const std::string& feature_id);
    bool evaluateSafetyGoals();
    int approvedRequirements() const;
    int activeFeatures() const;

private:
    std::vector<Requirement>             requirements_;
    std::vector<Feature>                 features_;
    std::vector<DiagnosticTroubleCode>   dtcs_;
    std::vector<SafetyGoal>              safety_goals_;
    double                               sim_time_;
};

} // namespace functional
} // namespace sdv
