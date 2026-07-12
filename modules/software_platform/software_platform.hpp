#pragma once
/**
 * @file software_platform.hpp
 * @brief Software Architecture – AUTOSAR, middleware, OS, SOA, lifecycle
 */

#include "sdv/common.hpp"
#include <string>
#include <vector>

namespace sdv {
namespace software {

enum class OSType        { None, ClassicAUTOSAR_OS, AdaptiveAUTOSAR, Linux, QNX, AndroidAutomotive };
enum class Middleware    { None, SOMEIP, DDS, MQTT, AdaptiveARA, COM };
enum class AppState      { Uninstalled, Installed, Starting, Running, Stopping, Failed };
enum class LifecyclePhase{ Development, Integration, Verification, Production, Maintenance, EOL };

struct AUTOSARStack {
    bool classic_available;
    bool adaptive_available;
    std::string classic_version;
    std::string adaptive_version;
};

struct SoftwareComponent {
    std::string name;
    std::string version;
    AppState    state;
    OSType      runtime;
    ASIL        asil;
};

struct ServiceInterface {
    std::string name;
    Middleware  protocol;
    bool        provided;
    bool        subscribed;
    int         method_count;
};

struct LifecycleStatus {
    LifecyclePhase phase;
    std::string    release_train;
    int            cicd_pipelines;
    bool           continuous_delivery;
};

class SoftwarePlatformModule : public ArchitectureModule {
public:
    SoftwarePlatformModule();

    bool initialize() override;
    bool start() override;
    void tick(double dt_sec) override;
    void stop() override;
    void shutdown() override;
    SignalMap exportSignals() const override;
    std::vector<std::string> capabilities() const override;

    bool deployApp(const std::string& name, const std::string& version, OSType runtime, ASIL asil);
    bool startApp(const std::string& name);
    bool stopApp(const std::string& name);
    void registerService(const std::string& name, Middleware protocol, int methods);
    void advanceLifecycle(LifecyclePhase phase);
    int runningApps() const;

    const AUTOSARStack& autosar() const { return autosar_; }
    const LifecycleStatus& lifecycle() const { return lifecycle_; }
    OSType primaryOS() const { return primary_os_; }

private:
    AUTOSARStack                   autosar_;
    std::vector<SoftwareComponent> apps_;
    std::vector<ServiceInterface>  services_;
    LifecycleStatus                lifecycle_;
    OSType                         primary_os_;
    double                         sim_time_;
};

} // namespace software
} // namespace sdv
