#pragma once
/**
 * @file infotainment.hpp
 * @brief Infotainment Architecture – IVI, digital cockpit, navigation, voice, apps
 */

#include "sdv/common.hpp"
#include <string>
#include <vector>

namespace sdv {
namespace infotainment {

enum class DisplayZone { DriverCluster, CenterIVI, HUD, Passenger, RearSeat };
enum class MediaSource { None, Radio, Bluetooth, USB, Streaming, Podcast };
enum class NavState    { Idle, Routing, Navigating, Recalculating, Arrived };

struct DisplayState {
    DisplayZone zone;
    bool        powered;
    int         brightness;   // 0-100
    std::string content;
};

struct NavigationState {
    NavState    state;
    std::string destination;
    double      remaining_km;
    int         eta_min;
};

struct VoiceAssistant {
    bool        listening;
    bool        speaking;
    std::string last_command;
    std::string last_response;
};

class InfotainmentModule : public ArchitectureModule {
public:
    InfotainmentModule();

    bool initialize() override;
    bool start() override;
    void tick(double dt_sec) override;
    void stop() override;
    void shutdown() override;
    SignalMap exportSignals() const override;
    std::vector<std::string> capabilities() const override;

    // Feature APIs
    void setMediaSource(MediaSource src);
    void setVolume(int vol);
    void startNavigation(const std::string& destination, double distance_km);
    void cancelNavigation();
    bool voiceCommand(const std::string& command);
    void setBrightness(DisplayZone zone, int brightness);
    void launchApp(const std::string& app_name);

    MediaSource mediaSource() const { return media_; }
    int volume() const { return volume_; }
    const NavigationState& navigation() const { return nav_; }
    const VoiceAssistant& voice() const { return voice_; }

private:
    std::vector<DisplayState> displays_;
    MediaSource               media_;
    int                       volume_;
    NavigationState           nav_;
    VoiceAssistant            voice_;
    std::vector<std::string>  running_apps_;
    double                    sim_time_;
};

} // namespace infotainment
} // namespace sdv
