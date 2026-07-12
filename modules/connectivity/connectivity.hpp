#pragma once
/**
 * @file connectivity.hpp
 * @brief Connectivity Architecture – 4G/5G, V2X, OTA, telematics, cloud
 */

#include "sdv/common.hpp"
#include <string>
#include <vector>
#include <queue>

namespace sdv {
namespace connectivity {

enum class NetworkType   { None, LTE_4G, NR_5G, WiFi, V2X };
enum class V2XMessageType{ CAM, DENM, BSM, SPaT, MAP };
enum class OTAState      { Idle, Checking, Downloading, Verifying, Installing, Complete, Failed };
enum class CloudLinkState{ Disconnected, Connecting, Connected, Degraded };

struct CellularModem {
    NetworkType type;
    int         rssi_dbm;
    double      downlink_mbps;
    double      uplink_mbps;
    bool        registered;
};

struct V2XMessage {
    V2XMessageType type;
    std::string    source_id;
    double         range_m;
    std::string    payload;
};

struct OTAUpdate {
    OTAState    state;
    std::string package_id;
    std::string version;
    double      progress_pct;
    std::size_t size_mb;
};

struct TelematicsEvent {
    std::string event_type;
    std::string payload;
    Timestamp   ts;
};

class ConnectivityModule : public ArchitectureModule {
public:
    ConnectivityModule();

    bool initialize() override;
    bool start() override;
    void tick(double dt_sec) override;
    void stop() override;
    void shutdown() override;
    SignalMap exportSignals() const override;
    std::vector<std::string> capabilities() const override;

    bool connectCloud();
    void disconnectCloud();
    void sendTelematics(const std::string& event_type, const std::string& payload);
    void receiveV2X(const V2XMessage& msg);
    bool startOTA(const std::string& package_id, const std::string& version, std::size_t size_mb);
    void switchNetwork(NetworkType type);

    const CellularModem& modem() const { return modem_; }
    CloudLinkState cloudState() const { return cloud_; }
    const OTAUpdate& ota() const { return ota_; }
    int pendingTelematics() const { return static_cast<int>(telematics_queue_.size()); }

private:
    CellularModem            modem_;
    CloudLinkState           cloud_;
    OTAUpdate                ota_;
    std::queue<TelematicsEvent> telematics_queue_;
    std::vector<V2XMessage>  v2x_inbox_;
    double                   sim_time_;
    int                      v2x_rx_count_;
};

} // namespace connectivity
} // namespace sdv
