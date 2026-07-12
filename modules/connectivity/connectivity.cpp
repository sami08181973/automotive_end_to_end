/**
 * @file connectivity.cpp
 * @brief Connectivity Architecture implementation
 */

#include "connectivity.hpp"

namespace sdv {
namespace connectivity {

ConnectivityModule::ConnectivityModule()
    : ArchitectureModule(ArchitectureDomain::Connectivity, "ConnectivityArchitecture", ASIL::B)
    , cloud_(CloudLinkState::Disconnected)
    , sim_time_(0.0)
    , v2x_rx_count_(0)
{
    modem_ = {NetworkType::None, -120, 0.0, 0.0, false};
    ota_   = {OTAState::Idle, "", "", 0.0, 0};
}

bool ConnectivityModule::initialize() {
    setStatus(SystemStatus::Initializing);
    modem_ = {NetworkType::NR_5G, -72, 450.0, 80.0, true};
    logEvent(domain_, "5G NR modem registered (RSSI -72 dBm)");
    logEvent(domain_, "V2X stack initialized (CAM/DENM/BSM)");
    logEvent(domain_, "Telematics ECU ready");
    setStatus(SystemStatus::Ready);
    return true;
}

bool ConnectivityModule::start() {
    setStatus(SystemStatus::Active);
    connectCloud();
    return true;
}

void ConnectivityModule::tick(double dt_sec) {
    if (status_ != SystemStatus::Active) return;
    sim_time_ += dt_sec;

    // Flush telematics to cloud
    if (cloud_ == CloudLinkState::Connected && !telematics_queue_.empty()) {
        auto evt = telematics_queue_.front();
        telematics_queue_.pop();
        logEvent(domain_, "Telematics uplink: " + evt.event_type);
    }

    // OTA progress simulation
    if (ota_.state == OTAState::Downloading) {
        ota_.progress_pct = std::min(100.0, ota_.progress_pct + 15.0 * dt_sec);
        if (ota_.progress_pct >= 100.0) {
            ota_.state = OTAState::Verifying;
            logEvent(domain_, "OTA download complete – verifying signature");
        }
    } else if (ota_.state == OTAState::Verifying) {
        ota_.state = OTAState::Installing;
        logEvent(domain_, "OTA signature OK – installing " + ota_.version);
    } else if (ota_.state == OTAState::Installing) {
        ota_.state = OTAState::Complete;
        logEvent(domain_, "OTA install complete: " + ota_.package_id + " v" + ota_.version);
    }

    // Periodic V2X CAM simulation
    if (static_cast<int>(sim_time_) % 2 == 0 && v2x_rx_count_ < static_cast<int>(sim_time_)) {
        receiveV2X({V2XMessageType::CAM, "RSU-42", 85.0, "intersection_status=green"});
        v2x_rx_count_ = static_cast<int>(sim_time_);
    }
}

void ConnectivityModule::stop() {
    disconnectCloud();
    setStatus(SystemStatus::Ready);
}

void ConnectivityModule::shutdown() {
    modem_.registered = false;
    setStatus(SystemStatus::Offline);
}

bool ConnectivityModule::connectCloud() {
    cloud_ = CloudLinkState::Connecting;
    logEvent(domain_, "Connecting to OEM cloud backend...");
    cloud_ = CloudLinkState::Connected;
    logEvent(domain_, "Cloud link established (MQTT/TLS)");
    return true;
}

void ConnectivityModule::disconnectCloud() {
    cloud_ = CloudLinkState::Disconnected;
    logEvent(domain_, "Cloud link disconnected");
}

void ConnectivityModule::sendTelematics(const std::string& event_type, const std::string& payload) {
    telematics_queue_.push({event_type, payload, Timestamp::now()});
}

void ConnectivityModule::receiveV2X(const V2XMessage& msg) {
    v2x_inbox_.push_back(msg);
    const char* types[] = {"CAM", "DENM", "BSM", "SPaT", "MAP"};
    logEvent(domain_, std::string("V2X RX ") + types[static_cast<int>(msg.type)]
             + " from " + msg.source_id + " @ " + std::to_string(static_cast<int>(msg.range_m)) + "m");
}

bool ConnectivityModule::startOTA(const std::string& package_id, const std::string& version, std::size_t size_mb) {
    if (cloud_ != CloudLinkState::Connected) {
        logWarn(domain_, "OTA aborted – cloud not connected");
        return false;
    }
    ota_ = {OTAState::Downloading, package_id, version, 0.0, size_mb};
    logEvent(domain_, "OTA started: " + package_id + " v" + version
             + " (" + std::to_string(size_mb) + " MB)");
    return true;
}

void ConnectivityModule::switchNetwork(NetworkType type) {
    modem_.type = type;
    if (type == NetworkType::NR_5G) {
        modem_.downlink_mbps = 450.0;
        modem_.rssi_dbm = -72;
    } else if (type == NetworkType::LTE_4G) {
        modem_.downlink_mbps = 80.0;
        modem_.rssi_dbm = -85;
    }
    logEvent(domain_, "Network switched");
}

SignalMap ConnectivityModule::exportSignals() const {
    SignalMap m;
    m["conn.rssi_dbm"] = {"conn.rssi_dbm", static_cast<double>(modem_.rssi_dbm), "dBm", Timestamp::now()};
    m["conn.downlink_mbps"] = {"conn.downlink_mbps", modem_.downlink_mbps, "Mbps", Timestamp::now()};
    m["conn.ota_progress"] = {"conn.ota_progress", ota_.progress_pct, "%", Timestamp::now()};
    m["conn.v2x_rx_count"] = {"conn.v2x_rx_count", static_cast<double>(v2x_inbox_.size()), "count", Timestamp::now()};
    return m;
}

std::vector<std::string> ConnectivityModule::capabilities() const {
    return {
        "4G LTE / 5G NR Cellular",
        "V2X (CAM, DENM, BSM, SPaT)",
        "OTA Software Updates",
        "Telematics & Remote Diagnostics",
        "Cloud Integration (MQTT/TLS)",
        "Wi-Fi Hotspot"
    };
}

} // namespace connectivity
} // namespace sdv
