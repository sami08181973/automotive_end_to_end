/**
 * @file infotainment.cpp
 * @brief Infotainment Architecture implementation
 */

#include "infotainment.hpp"
#include <sstream>

namespace sdv {
namespace infotainment {

InfotainmentModule::InfotainmentModule()
    : ArchitectureModule(ArchitectureDomain::Infotainment, "InfotainmentArchitecture", ASIL::B)
    , media_(MediaSource::None)
    , volume_(40)
    , sim_time_(0.0)
{
    nav_ = {NavState::Idle, "", 0.0, 0};
    voice_ = {false, false, "", ""};
}

bool InfotainmentModule::initialize() {
    setStatus(SystemStatus::Initializing);

    displays_ = {
        {DisplayZone::DriverCluster, true,  80, "Cluster: Speed/ADAS"},
        {DisplayZone::CenterIVI,     true,  70, "IVI Home"},
        {DisplayZone::HUD,           true,  60, "HUD Ready"},
        {DisplayZone::Passenger,     false, 50, "Passenger Off"},
        {DisplayZone::RearSeat,      false, 50, "RSE Off"}
    };

    running_apps_ = {"SystemUI", "ClimateWidget"};
    logEvent(domain_, "Digital cockpit initialized (5 display zones)");
    logEvent(domain_, "IVI middleware + HMI framework ready");
    setStatus(SystemStatus::Ready);
    return true;
}

bool InfotainmentModule::start() {
    setStatus(SystemStatus::Active);
    setMediaSource(MediaSource::Streaming);
    logEvent(domain_, "App ecosystem online – streaming media started");
    return true;
}

void InfotainmentModule::tick(double dt_sec) {
    if (status_ != SystemStatus::Active) return;
    sim_time_ += dt_sec;

    // Simulate navigation progress
    if (nav_.state == NavState::Navigating && nav_.remaining_km > 0.0) {
        double speed_kmh = 60.0;
        nav_.remaining_km = std::max(0.0, nav_.remaining_km - (speed_kmh / 3600.0) * dt_sec);
        nav_.eta_min = static_cast<int>(nav_.remaining_km / (speed_kmh / 60.0));
        if (nav_.remaining_km <= 0.01) {
            nav_.state = NavState::Arrived;
            nav_.remaining_km = 0.0;
            logEvent(domain_, "Navigation: Arrived at " + nav_.destination);
        }
    }

    // Voice assistant idle timeout
    if (voice_.speaking && sim_time_ > 2.0) {
        voice_.speaking = false;
    }
}

void InfotainmentModule::stop() {
    media_ = MediaSource::None;
    setStatus(SystemStatus::Ready);
}

void InfotainmentModule::shutdown() {
    displays_.clear();
    running_apps_.clear();
    setStatus(SystemStatus::Offline);
}

void InfotainmentModule::setMediaSource(MediaSource src) {
    media_ = src;
    const char* names[] = {"None", "Radio", "Bluetooth", "USB", "Streaming", "Podcast"};
    logEvent(domain_, std::string("Media source -> ") + names[static_cast<int>(src)]);
}

void InfotainmentModule::setVolume(int vol) {
    volume_ = clamp(vol, 0, 100);
}

void InfotainmentModule::startNavigation(const std::string& destination, double distance_km) {
    nav_.state = NavState::Routing;
    nav_.destination = destination;
    nav_.remaining_km = distance_km;
    nav_.eta_min = static_cast<int>(distance_km / 60.0 * 60.0);
    logEvent(domain_, "Routing to: " + destination);
    nav_.state = NavState::Navigating;
    for (auto& d : displays_) {
        if (d.zone == DisplayZone::CenterIVI || d.zone == DisplayZone::HUD)
            d.content = "Nav: " + destination;
    }
}

void InfotainmentModule::cancelNavigation() {
    nav_ = {NavState::Idle, "", 0.0, 0};
    logEvent(domain_, "Navigation cancelled");
}

bool InfotainmentModule::voiceCommand(const std::string& command) {
    voice_.listening = true;
    voice_.last_command = command;
    logEvent(domain_, "Voice: \"" + command + "\"");

    if (command.find("navigate") != std::string::npos ||
        command.find("Navigate") != std::string::npos) {
        voice_.last_response = "Starting navigation";
        startNavigation("City Center", 12.5);
    } else if (command.find("volume") != std::string::npos) {
        setVolume(55);
        voice_.last_response = "Volume set to 55";
    } else if (command.find("climate") != std::string::npos) {
        voice_.last_response = "Opening climate controls";
        launchApp("Climate");
    } else {
        voice_.last_response = "Command acknowledged";
    }

    voice_.listening = false;
    voice_.speaking = true;
    logEvent(domain_, "Assistant: \"" + voice_.last_response + "\"");
    return true;
}

void InfotainmentModule::setBrightness(DisplayZone zone, int brightness) {
    for (auto& d : displays_) {
        if (d.zone == zone) {
            d.brightness = clamp(brightness, 0, 100);
            break;
        }
    }
}

void InfotainmentModule::launchApp(const std::string& app_name) {
    running_apps_.push_back(app_name);
    logEvent(domain_, "App launched: " + app_name);
}

SignalMap InfotainmentModule::exportSignals() const {
    SignalMap m;
    m["ivi.volume"] = {"ivi.volume", static_cast<double>(volume_), "%", Timestamp::now()};
    m["ivi.nav_remaining_km"] = {"ivi.nav_remaining_km", nav_.remaining_km, "km", Timestamp::now()};
    m["ivi.apps_running"] = {"ivi.apps_running", static_cast<double>(running_apps_.size()), "count", Timestamp::now()};
    return m;
}

std::vector<std::string> InfotainmentModule::capabilities() const {
    return {
        "IVI / Digital Cockpit",
        "Multi-zone Displays (Cluster, IVI, HUD, Passenger, RSE)",
        "Navigation & Routing",
        "Voice Assistant",
        "Media & App Ecosystem",
        "HMI Framework"
    };
}

} // namespace infotainment
} // namespace sdv
