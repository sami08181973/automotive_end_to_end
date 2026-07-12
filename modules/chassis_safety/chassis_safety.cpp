/**
 * @file chassis_safety.cpp
 * @brief Chassis & Safety Architecture implementation
 */

#include "chassis_safety.hpp"

namespace sdv {
namespace chassis {

ChassisSafetyModule::ChassisSafetyModule()
    : ArchitectureModule(ArchitectureDomain::ChassisSafety, "ChassisSafetyArchitecture", ASIL::D)
    , yaw_rate_(0.0)
    , sim_time_(0.0)
{
    brakes_ = {BrakeMode::None, 0.0, 0.0, true, true};
    steering_ = {SteeringMode::EPS_Assist, 0.0, 0.0, true};
    restraints_ = {AirbagStatus::Armed, AirbagStatus::Armed,
                   AirbagStatus::Armed, AirbagStatus::Armed, true, false};
}

bool ChassisSafetyModule::initialize() {
    setStatus(SystemStatus::Initializing);

    for (auto& w : wheels_) {
        w = {600.0, 0.0, false, 230.0, 35.0};
    }

    logEvent(domain_, "ABS/ESC controllers online (ASIL-D)");
    logEvent(domain_, "TPMS + EPS + SRS (airbags) armed");
    setStatus(SystemStatus::Ready);
    return true;
}

bool ChassisSafetyModule::start() {
    setStatus(SystemStatus::Active);
    setSteeringMode(SteeringMode::LaneKeep);
    return true;
}

void ChassisSafetyModule::tick(double dt_sec) {
    if (status_ != SystemStatus::Active) return;
    sim_time_ += dt_sec;

    // Apply brake pressure from pedal
    brakes_.master_cylinder_bar = brakes_.pedal_pct * 1.2;
    for (auto& w : wheels_) {
        w.brake_pressure_bar = brakes_.master_cylinder_bar;
        if (!w.abs_active) {
            w.speed_rpm = std::max(0.0, w.speed_rpm - brakes_.pedal_pct * 5.0 * dt_sec);
        }
    }

    if (brakes_.abs_enabled) runABS(dt_sec);
    if (brakes_.esc_enabled) runESC(dt_sec);

    // Tire temp rises with braking
    for (auto& w : wheels_) {
        if (w.brake_pressure_bar > 10.0)
            w.tire_temp_c += 0.5 * dt_sec;
    }
}

void ChassisSafetyModule::stop() {
    brakes_.pedal_pct = 0.0;
    brakes_.mode = BrakeMode::None;
    setStatus(SystemStatus::Ready);
}

void ChassisSafetyModule::shutdown() {
    restraints_.driver = AirbagStatus::Disabled;
    setStatus(SystemStatus::Offline);
}

void ChassisSafetyModule::setBrakePedal(double pct) {
    brakes_.pedal_pct = clamp(pct, 0.0, 100.0);
    if (pct > 0.1) {
        brakes_.mode = BrakeMode::Blended;
        logEvent(domain_, "Brake pedal " + std::to_string(static_cast<int>(pct)) + "% – blended regen+hydraulic");
    } else {
        brakes_.mode = BrakeMode::None;
    }
}

void ChassisSafetyModule::setSteeringAngle(double deg) {
    steering_.angle_deg = clamp(deg, -470.0, 470.0);
    steering_.torque_nm = std::abs(deg) * 0.02;
}

void ChassisSafetyModule::setSteeringMode(SteeringMode mode) {
    steering_.mode = mode;
    const char* names[] = {"Manual", "EPS_Assist", "LaneKeep", "Automated"};
    logEvent(domain_, std::string("Steering mode -> ") + names[static_cast<int>(mode)]);
}

void ChassisSafetyModule::simulateWheelLock(int wheel_idx) {
    if (wheel_idx < 0 || wheel_idx > 3) return;
    wheels_[wheel_idx].speed_rpm = 0.0;
    wheels_[wheel_idx].brake_pressure_bar = 100.0;
    logWarn(domain_, "Wheel " + std::to_string(wheel_idx) + " lockup detected");
}

void ChassisSafetyModule::simulateLowTirePressure(int wheel_idx) {
    if (wheel_idx < 0 || wheel_idx > 3) return;
    wheels_[wheel_idx].tire_pressure_kpa = 160.0;
    logWarn(domain_, "TPMS warning – wheel " + std::to_string(wheel_idx)
             + " pressure " + std::to_string(static_cast<int>(wheels_[wheel_idx].tire_pressure_kpa)) + " kPa");
}

void ChassisSafetyModule::simulateCrash(bool frontal) {
    restraints_.crash_detected = true;
    if (frontal) {
        restraints_.driver = AirbagStatus::Deployed;
        restraints_.passenger = AirbagStatus::Deployed;
        logEvent(domain_, "SRS DEPLOY – frontal airbags + pretensioners");
    } else {
        restraints_.side_left = AirbagStatus::Deployed;
        logEvent(domain_, "SRS DEPLOY – side curtain airbag");
    }
}

bool ChassisSafetyModule::checkTPMS() const {
    for (const auto& w : wheels_) {
        if (w.tire_pressure_kpa < 190.0) return false;
    }
    return true;
}

void ChassisSafetyModule::runABS(double dt) {
    (void)dt;
    bool any_abs = false;
    for (auto& w : wheels_) {
        // Detect lockup: high pressure + near-zero wheel speed while vehicle moving
        double avg_speed = 0.0;
        for (const auto& x : wheels_) avg_speed += x.speed_rpm;
        avg_speed /= 4.0;

        if (w.brake_pressure_bar > 50.0 && w.speed_rpm < avg_speed * 0.3 && avg_speed > 50.0) {
            w.abs_active = true;
            w.brake_pressure_bar *= 0.5; // modulate
            any_abs = true;
        } else {
            w.abs_active = false;
        }
    }
    if (any_abs) {
        brakes_.mode = BrakeMode::ABS;
        logEvent(domain_, "ABS active – brake pressure modulation");
    }
}

void ChassisSafetyModule::runESC(double dt) {
    (void)dt;
    // Simple yaw stability: if steering large and speed high, apply selective braking
    double avg_speed = 0.0;
    for (const auto& w : wheels_) avg_speed += w.speed_rpm;
    avg_speed /= 4.0;

    if (std::abs(steering_.angle_deg) > 90.0 && avg_speed > 400.0) {
        brakes_.mode = BrakeMode::ESC;
        yaw_rate_ = steering_.angle_deg * 0.01;
        logEvent(domain_, "ESC intervention – yaw rate correction");
    }
}

SignalMap ChassisSafetyModule::exportSignals() const {
    SignalMap m;
    m["chassis.brake_pedal_pct"] = {"chassis.brake_pedal_pct", brakes_.pedal_pct, "%", Timestamp::now()};
    m["chassis.steer_angle_deg"] = {"chassis.steer_angle_deg", steering_.angle_deg, "deg", Timestamp::now()};
    m["chassis.tpms_ok"] = {"chassis.tpms_ok", checkTPMS() ? 1.0 : 0.0, "bool", Timestamp::now()};
    m["chassis.yaw_rate"] = {"chassis.yaw_rate", yaw_rate_, "rad/s", Timestamp::now()};
    return m;
}

std::vector<std::string> ChassisSafetyModule::capabilities() const {
    return {
        "ABS (Anti-lock Braking)",
        "ESC (Electronic Stability Control)",
        "TPMS (Tire Pressure Monitoring)",
        "SRS Airbags & Pretensioners",
        "EPS Electric Power Steering",
        "Blended Regenerative Braking"
    };
}

} // namespace chassis
} // namespace sdv
