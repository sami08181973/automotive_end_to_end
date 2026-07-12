/**
 * @file software_platform.cpp
 * @brief Software Architecture implementation
 */

#include "software_platform.hpp"

namespace sdv {
namespace software {

SoftwarePlatformModule::SoftwarePlatformModule()
    : ArchitectureModule(ArchitectureDomain::SoftwarePlatform, "SoftwareArchitecture", ASIL::B)
    , primary_os_(OSType::AdaptiveAUTOSAR)
    , sim_time_(0.0)
{
    autosar_ = {true, true, "R22-11", "R22-11"};
    lifecycle_ = {LifecyclePhase::Production, "SDV-2026.Q2", 8, true};
}

bool SoftwarePlatformModule::initialize() {
    setStatus(SystemStatus::Initializing);

    logEvent(domain_, std::string("AUTOSAR Classic ") + autosar_.classic_version
             + " + Adaptive " + autosar_.adaptive_version);
    logEvent(domain_, "Middleware: SOME/IP, DDS, ARA::COM");

    registerService("VehicleSpeed", Middleware::SOMEIP, 3);
    registerService("CabinClimate", Middleware::SOMEIP, 5);
    registerService("PerceptionObjects", Middleware::DDS, 4);
    registerService("CloudTelemetry", Middleware::MQTT, 2);
    registerService("PowertrainTorque", Middleware::AdaptiveARA, 6);

    setStatus(SystemStatus::Ready);
    return true;
}

bool SoftwarePlatformModule::start() {
    setStatus(SystemStatus::Active);

    deployApp("ClusterHMI",      "3.2.0", OSType::AdaptiveAUTOSAR, ASIL::B);
    deployApp("ADASPerception",  "2.1.0", OSType::AdaptiveAUTOSAR, ASIL::D);
    deployApp("BodyGateway",     "1.8.0", OSType::ClassicAUTOSAR_OS, ASIL::B);
    deployApp("InfotainmentUI",  "5.0.1", OSType::AndroidAutomotive, ASIL::QM);
    deployApp("DataCollector",   "1.4.0", OSType::Linux, ASIL::A);

    startApp("ClusterHMI");
    startApp("ADASPerception");
    startApp("BodyGateway");
    startApp("InfotainmentUI");
    startApp("DataCollector");

    advanceLifecycle(LifecyclePhase::Production);
    return true;
}

void SoftwarePlatformModule::tick(double dt_sec) {
    if (status_ != SystemStatus::Active) return;
    sim_time_ += dt_sec;
}

void SoftwarePlatformModule::stop() {
    for (auto& a : apps_) {
        if (a.state == AppState::Running) a.state = AppState::Stopping;
        a.state = AppState::Installed;
    }
    setStatus(SystemStatus::Ready);
}

void SoftwarePlatformModule::shutdown() {
    apps_.clear();
    services_.clear();
    setStatus(SystemStatus::Offline);
}

bool SoftwarePlatformModule::deployApp(const std::string& name, const std::string& version,
                                       OSType runtime, ASIL asil) {
    apps_.push_back({name, version, AppState::Installed, runtime, asil});
    logEvent(domain_, "Deployed: " + name + " v" + version + " [" + asilName(asil) + "]");
    return true;
}

bool SoftwarePlatformModule::startApp(const std::string& name) {
    for (auto& a : apps_) {
        if (a.name == name) {
            a.state = AppState::Starting;
            a.state = AppState::Running;
            logEvent(domain_, "App RUNNING: " + name);
            return true;
        }
    }
    return false;
}

bool SoftwarePlatformModule::stopApp(const std::string& name) {
    for (auto& a : apps_) {
        if (a.name == name) {
            a.state = AppState::Stopping;
            a.state = AppState::Installed;
            logEvent(domain_, "App STOPPED: " + name);
            return true;
        }
    }
    return false;
}

void SoftwarePlatformModule::registerService(const std::string& name, Middleware protocol, int methods) {
    services_.push_back({name, protocol, true, false, methods});
    const char* mw[] = {"None", "SOME/IP", "DDS", "MQTT", "ARA::COM", "COM"};
    logEvent(domain_, std::string("SOA service: ") + name + " via " + mw[static_cast<int>(protocol)]);
}

void SoftwarePlatformModule::advanceLifecycle(LifecyclePhase phase) {
    lifecycle_.phase = phase;
    const char* names[] = {"Development", "Integration", "Verification",
                           "Production", "Maintenance", "EOL"};
    logEvent(domain_, std::string("Software lifecycle -> ") + names[static_cast<int>(phase)]
             + " (train: " + lifecycle_.release_train + ")");
}

int SoftwarePlatformModule::runningApps() const {
    int n = 0;
    for (const auto& a : apps_) if (a.state == AppState::Running) ++n;
    return n;
}

SignalMap SoftwarePlatformModule::exportSignals() const {
    SignalMap m;
    m["sw.apps_running"] = {"sw.apps_running", static_cast<double>(runningApps()), "count", Timestamp::now()};
    m["sw.services"] = {"sw.services", static_cast<double>(services_.size()), "count", Timestamp::now()};
    m["sw.cicd_pipelines"] = {"sw.cicd_pipelines", static_cast<double>(lifecycle_.cicd_pipelines), "count", Timestamp::now()};
    return m;
}

std::vector<std::string> SoftwarePlatformModule::capabilities() const {
    return {
        "AUTOSAR Classic & Adaptive",
        "SOA Middleware (SOME/IP, DDS, MQTT)",
        "Multi-OS (QNX, Linux, Android AAOS)",
        "Hypervisor Guest Partitioning (FFI)",
        "Application Lifecycle Management",
        "CI/CD Continuous Delivery",
        "Software Component Deployment"
    };
}

} // namespace software
} // namespace sdv
