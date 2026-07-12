#pragma once
/**
 * @file body_comfort.hpp
 * @brief Body & Comfort Architecture – HVAC, lighting, seats, keyless, BCM
 */

#include "sdv/common.hpp"
#include <string>
#include <vector>

namespace sdv {
namespace body {

enum class LightMode   { Off, Position, LowBeam, HighBeam, Auto, DRL };
enum class DoorState   { Locked, Unlocked, Open, Ajar };
enum class SeatZone    { Driver, Passenger, RearLeft, RearRight };

struct HVACState {
    double cabin_temp_c;
    double target_temp_c;
    int    fan_speed;       // 0-7
    bool   ac_on;
    bool   recirculate;
    bool   defrost;
};

struct SeatState {
    SeatZone zone;
    int      position;      // 0-100
    int      heat_level;    // 0-3
    int      vent_level;    // 0-3
    bool     occupied;
};

struct KeylessEntry {
    bool   fob_in_range;
    bool   passive_entry;
    bool   passive_start;
    bool   welcome_light;
};

class BodyComfortModule : public ArchitectureModule {
public:
    BodyComfortModule();

    bool initialize() override;
    bool start() override;
    void tick(double dt_sec) override;
    void stop() override;
    void shutdown() override;
    SignalMap exportSignals() const override;
    std::vector<std::string> capabilities() const override;

    void setCabinTarget(double temp_c);
    void setFanSpeed(int speed);
    void setLightMode(LightMode mode);
    void lockDoors();
    void unlockDoors();
    void setSeatHeat(SeatZone zone, int level);
    void keyFobApproach(bool in_range);
    void welcomeSequence();

    const HVACState& hvac() const { return hvac_; }
    LightMode lights() const { return lights_; }

private:
    HVACState              hvac_;
    LightMode              lights_;
    std::vector<DoorState> doors_;   // FL, FR, RL, RR, Trunk
    std::vector<SeatState> seats_;
    KeylessEntry           keyless_;
    double                 sim_time_;
};

} // namespace body
} // namespace sdv
