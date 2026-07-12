/**
 * @file body_comfort.cpp
 * @brief Body & Comfort Architecture implementation
 */

#include "body_comfort.hpp"

namespace sdv {
namespace body {

BodyComfortModule::BodyComfortModule()
    : ArchitectureModule(ArchitectureDomain::BodyComfort, "BodyComfortArchitecture", ASIL::A)
    , lights_(LightMode::Off)
    , sim_time_(0.0)
{
    hvac_ = {22.0, 22.0, 2, false, false, false};
    keyless_ = {false, true, true, false};
}

bool BodyComfortModule::initialize() {
    setStatus(SystemStatus::Initializing);

    doors_ = {DoorState::Locked, DoorState::Locked, DoorState::Locked,
              DoorState::Locked, DoorState::Locked};

    seats_ = {
        {SeatZone::Driver,    45, 0, 0, false},
        {SeatZone::Passenger, 40, 0, 0, false},
        {SeatZone::RearLeft,  50, 0, 0, false},
        {SeatZone::RearRight, 50, 0, 0, false}
    };

    logEvent(domain_, "BCM online – doors, lighting, HVAC, seats");
    logEvent(domain_, "Keyless entry / passive start ready");
    setStatus(SystemStatus::Ready);
    return true;
}

bool BodyComfortModule::start() {
    setStatus(SystemStatus::Active);
    keyFobApproach(true);
    setLightMode(LightMode::Auto);
    setCabinTarget(21.5);
    hvac_.ac_on = true;
    seats_[0].occupied = true;
    return true;
}

void BodyComfortModule::tick(double dt_sec) {
    if (status_ != SystemStatus::Active) return;
    sim_time_ += dt_sec;

    // HVAC thermal model (simple 1st-order)
    double err = hvac_.target_temp_c - hvac_.cabin_temp_c;
    double rate = (hvac_.ac_on ? 0.15 : 0.05) * hvac_.fan_speed;
    hvac_.cabin_temp_c += err * rate * dt_sec;
}

void BodyComfortModule::stop() {
    setLightMode(LightMode::Off);
    setStatus(SystemStatus::Ready);
}

void BodyComfortModule::shutdown() {
    doors_.clear();
    seats_.clear();
    setStatus(SystemStatus::Offline);
}

void BodyComfortModule::setCabinTarget(double temp_c) {
    hvac_.target_temp_c = clamp(temp_c, 16.0, 30.0);
    logEvent(domain_, "HVAC target -> " + std::to_string(hvac_.target_temp_c) + " C");
}

void BodyComfortModule::setFanSpeed(int speed) {
    hvac_.fan_speed = static_cast<int>(clamp(speed, 0, 7));
}

void BodyComfortModule::setLightMode(LightMode mode) {
    lights_ = mode;
    const char* names[] = {"Off", "Position", "LowBeam", "HighBeam", "Auto", "DRL"};
    logEvent(domain_, std::string("Lighting -> ") + names[static_cast<int>(mode)]);
}

void BodyComfortModule::lockDoors() {
    for (auto& d : doors_) d = DoorState::Locked;
    logEvent(domain_, "All doors locked (BCM)");
}

void BodyComfortModule::unlockDoors() {
    for (auto& d : doors_) d = DoorState::Unlocked;
    logEvent(domain_, "All doors unlocked (BCM)");
}

void BodyComfortModule::setSeatHeat(SeatZone zone, int level) {
    for (auto& s : seats_) {
        if (s.zone == zone) {
            s.heat_level = static_cast<int>(clamp(level, 0, 3));
            logEvent(domain_, "Seat heat zone " + std::to_string(static_cast<int>(zone))
                     + " -> level " + std::to_string(s.heat_level));
            break;
        }
    }
}

void BodyComfortModule::keyFobApproach(bool in_range) {
    keyless_.fob_in_range = in_range;
    if (in_range && keyless_.passive_entry) {
        unlockDoors();
        welcomeSequence();
    } else if (!in_range) {
        lockDoors();
        keyless_.welcome_light = false;
    }
}

void BodyComfortModule::welcomeSequence() {
    keyless_.welcome_light = true;
    setLightMode(LightMode::Position);
    logEvent(domain_, "Welcome sequence – puddle lamps + ambient lighting");
}

SignalMap BodyComfortModule::exportSignals() const {
    SignalMap m;
    m["body.cabin_temp_c"] = {"body.cabin_temp_c", hvac_.cabin_temp_c, "C", Timestamp::now()};
    m["body.target_temp_c"] = {"body.target_temp_c", hvac_.target_temp_c, "C", Timestamp::now()};
    m["body.fan_speed"] = {"body.fan_speed", static_cast<double>(hvac_.fan_speed), "level", Timestamp::now()};
    m["body.fob_in_range"] = {"body.fob_in_range", keyless_.fob_in_range ? 1.0 : 0.0, "bool", Timestamp::now()};
    return m;
}

std::vector<std::string> BodyComfortModule::capabilities() const {
    return {
        "HVAC Climate Control",
        "Exterior / Interior Lighting",
        "Power Seat Control (Heat/Vent)",
        "Keyless Entry & Passive Start",
        "Body Control Module (BCM)",
        "Door / Window / Mirror Actuation"
    };
}

} // namespace body
} // namespace sdv
