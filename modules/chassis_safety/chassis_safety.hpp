#pragma once
/**
 * @file chassis_safety.hpp
 * @brief Chassis & Safety Architecture – ABS, ESC, TPMS, airbags, steering/braking
 */

#include "sdv/common.hpp"
#include <string>
#include <vector>
#include <array>

namespace sdv {
namespace chassis {

enum class BrakeMode    { None, Regenerative, Hydraulic, Blended, ABS, ESC };
enum class SteeringMode { Manual, EPS_Assist, LaneKeep, Automated };
enum class AirbagStatus { Armed, Deployed, Fault, Disabled };

struct WheelState {
    double speed_rpm;
    double brake_pressure_bar;
    bool   abs_active;
    double tire_pressure_kpa;
    double tire_temp_c;
};

struct BrakeSystem {
    BrakeMode mode;
    double    pedal_pct;
    double    master_cylinder_bar;
    bool      abs_enabled;
    bool      esc_enabled;
};

struct SteeringSystem {
    SteeringMode mode;
    double       angle_deg;
    double       torque_nm;
    bool         eps_ok;
};

struct RestraintSystem {
    AirbagStatus driver;
    AirbagStatus passenger;
    AirbagStatus side_left;
    AirbagStatus side_right;
    bool         pretensioner_ready;
    bool         crash_detected;
};

class ChassisSafetyModule : public ArchitectureModule {
public:
    ChassisSafetyModule();

    bool initialize() override;
    bool start() override;
    void tick(double dt_sec) override;
    void stop() override;
    void shutdown() override;
    SignalMap exportSignals() const override;
    std::vector<std::string> capabilities() const override;

    void setBrakePedal(double pct);
    void setSteeringAngle(double deg);
    void setSteeringMode(SteeringMode mode);
    void simulateWheelLock(int wheel_idx);
    void simulateLowTirePressure(int wheel_idx);
    void simulateCrash(bool frontal);
    bool checkTPMS() const;

    const BrakeSystem& brakes() const { return brakes_; }
    const SteeringSystem& steering() const { return steering_; }
    const RestraintSystem& restraints() const { return restraints_; }

private:
    void runABS(double dt);
    void runESC(double dt);

    BrakeSystem                brakes_;
    SteeringSystem             steering_;
    RestraintSystem            restraints_;
    std::array<WheelState, 4>  wheels_;  // FL, FR, RL, RR
    double                     yaw_rate_;
    double                     sim_time_;
};

} // namespace chassis
} // namespace sdv
