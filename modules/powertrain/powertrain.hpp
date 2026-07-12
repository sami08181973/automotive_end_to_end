#pragma once
/**
 * @file powertrain.hpp
 * @brief Powertrain Architecture – motor control, BMS, inverter, thermal
 */

#include "sdv/common.hpp"
#include <string>
#include <vector>

namespace sdv {
namespace powertrain {

enum class PropulsionType { ICE, HEV, PHEV, BEV, FCEV };
enum class DriveMode      { Eco, Comfort, Sport, OffRoad, Tow };
enum class BMSState       { Idle, Charging, Discharging, Balancing, Fault, ThermalLimit };

struct BatteryPack {
    double soc_pct;          // State of Charge
    double soh_pct;          // State of Health
    double voltage_v;
    double current_a;
    double temp_c;
    double max_charge_kw;
    double max_discharge_kw;
    BMSState state;
    int    cell_count;
};

struct Inverter {
    double dc_voltage_v;
    double ac_current_a;
    double switching_khz;
    double efficiency_pct;
    double temp_c;
    bool   enabled;
};

struct ElectricMotor {
    double torque_nm;
    double speed_rpm;
    double power_kw;
    double temp_c;
    bool   enabled;
};

struct ThermalLoop {
    double coolant_temp_c;
    double flow_lpm;
    bool   pump_on;
    bool   radiator_fan;
};

class PowertrainModule : public ArchitectureModule {
public:
    PowertrainModule();

    bool initialize() override;
    bool start() override;
    void tick(double dt_sec) override;
    void stop() override;
    void shutdown() override;
    SignalMap exportSignals() const override;
    std::vector<std::string> capabilities() const override;

    void setDriveMode(DriveMode mode);
    void requestTorque(double torque_nm);
    void setChargeRequest(bool charging, double power_kw);
    void emergencyPowerLimit();

    const BatteryPack& battery() const { return bms_; }
    const ElectricMotor& motor() const { return motor_; }
    DriveMode driveMode() const { return drive_mode_; }
    PropulsionType propulsion() const { return propulsion_; }

private:
    void updateBMS(double dt);
    void updateInverter(double dt);
    void updateThermal(double dt);

    DriveMode      drive_mode_;
    PropulsionType propulsion_;  // BEV demo default; exposed via capabilities
    BatteryPack    bms_;
    Inverter       inverter_;
    ElectricMotor  motor_;
    ThermalLoop    thermal_;
    double         torque_request_;
    double         sim_time_;
};

} // namespace powertrain
} // namespace sdv
