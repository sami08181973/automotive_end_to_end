/**
 * @file powertrain.cpp
 * @brief Powertrain Architecture implementation
 */

#include "powertrain.hpp"

namespace sdv {
namespace powertrain {

PowertrainModule::PowertrainModule()
    : ArchitectureModule(ArchitectureDomain::Powertrain, "PowertrainArchitecture", ASIL::C)
    , drive_mode_(DriveMode::Comfort)
    , propulsion_(PropulsionType::BEV)
    , torque_request_(0.0)
    , sim_time_(0.0)
{
    bms_ = {75.0, 98.0, 400.0, 0.0, 28.0, 150.0, 250.0, BMSState::Idle, 96};
    inverter_ = {400.0, 0.0, 10.0, 97.0, 40.0, false};
    motor_ = {0.0, 0.0, 0.0, 35.0, false};
    thermal_ = {30.0, 0.0, false, false};
}

bool PowertrainModule::initialize() {
    setStatus(SystemStatus::Initializing);
    logEvent(domain_, "BEV powertrain init – BMS (96s), inverter, e-motor");
    logEvent(domain_, "Thermal management loops ready");
    setStatus(SystemStatus::Ready);
    return true;
}

bool PowertrainModule::start() {
    setStatus(SystemStatus::Active);
    inverter_.enabled = true;
    motor_.enabled = true;
    bms_.state = BMSState::Discharging;
    thermal_.pump_on = true;
    thermal_.flow_lpm = 12.0;
    setDriveMode(DriveMode::Comfort);
    requestTorque(80.0);
    return true;
}

void PowertrainModule::tick(double dt_sec) {
    if (status_ != SystemStatus::Active) return;
    sim_time_ += dt_sec;

    updateBMS(dt_sec);
    updateInverter(dt_sec);
    updateThermal(dt_sec);

    // Motor dynamics
    double torque_limit = (drive_mode_ == DriveMode::Sport) ? 350.0 :
                          (drive_mode_ == DriveMode::Eco)   ? 150.0 : 250.0;
    motor_.torque_nm = clamp(torque_request_, -100.0, torque_limit);
    motor_.speed_rpm = lerp(motor_.speed_rpm, motor_.torque_nm * 12.0, 0.05);
    motor_.power_kw = (motor_.torque_nm * motor_.speed_rpm * 2.0 * 3.14159 / 60.0) / 1000.0;
}

void PowertrainModule::stop() {
    torque_request_ = 0.0;
    motor_.enabled = false;
    inverter_.enabled = false;
    bms_.state = BMSState::Idle;
    setStatus(SystemStatus::Ready);
}

void PowertrainModule::shutdown() {
    setStatus(SystemStatus::Offline);
}

void PowertrainModule::setDriveMode(DriveMode mode) {
    drive_mode_ = mode;
    const char* names[] = {"Eco", "Comfort", "Sport", "OffRoad", "Tow"};
    logEvent(domain_, std::string("Drive mode -> ") + names[static_cast<int>(mode)]);
}

void PowertrainModule::requestTorque(double torque_nm) {
    torque_request_ = torque_nm;
}

void PowertrainModule::setChargeRequest(bool charging, double power_kw) {
    if (charging) {
        bms_.state = BMSState::Charging;
        bms_.current_a = -(power_kw * 1000.0) / bms_.voltage_v;
        logEvent(domain_, "DC charge session started @ " + std::to_string(power_kw) + " kW");
    } else {
        bms_.state = BMSState::Discharging;
        bms_.current_a = 0.0;
    }
}

void PowertrainModule::emergencyPowerLimit() {
    torque_request_ = std::min(torque_request_, 50.0);
    bms_.state = BMSState::ThermalLimit;
    logWarn(domain_, "Emergency power limit engaged");
}

void PowertrainModule::updateBMS(double dt) {
    if (bms_.state == BMSState::Discharging && motor_.power_kw > 0.1) {
        bms_.current_a = (motor_.power_kw * 1000.0) / bms_.voltage_v;
        double energy_kwh = motor_.power_kw * dt / 3600.0;
        double capacity_kwh = 75.0;
        bms_.soc_pct = std::max(0.0, bms_.soc_pct - (energy_kwh / capacity_kwh) * 100.0);
    } else if (bms_.state == BMSState::Charging) {
        double energy_kwh = std::abs(bms_.current_a * bms_.voltage_v / 1000.0) * dt / 3600.0;
        bms_.soc_pct = std::min(100.0, bms_.soc_pct + (energy_kwh / 75.0) * 100.0);
    }

    if (bms_.temp_c > 45.0) {
        bms_.state = BMSState::ThermalLimit;
        logWarn(domain_, "BMS thermal limit – derating power");
    }
}

void PowertrainModule::updateInverter(double dt) {
    (void)dt;
    if (!inverter_.enabled) return;
    inverter_.dc_voltage_v = bms_.voltage_v;
    inverter_.ac_current_a = std::abs(motor_.power_kw * 1000.0 / (inverter_.dc_voltage_v * 0.97 + 1e-6));
    inverter_.temp_c = lerp(inverter_.temp_c, 40.0 + inverter_.ac_current_a * 0.05, 0.1);
}

void PowertrainModule::updateThermal(double dt) {
    (void)dt;
    if (!thermal_.pump_on) return;
    double heat = motor_.power_kw * 0.03 + inverter_.temp_c * 0.01;
    thermal_.coolant_temp_c = lerp(thermal_.coolant_temp_c, 25.0 + heat, 0.05);
    thermal_.radiator_fan = thermal_.coolant_temp_c > 40.0;
    motor_.temp_c = thermal_.coolant_temp_c + 5.0;
    bms_.temp_c = lerp(bms_.temp_c, thermal_.coolant_temp_c - 2.0, 0.02);
}

SignalMap PowertrainModule::exportSignals() const {
    SignalMap m;
    m["pt.soc_pct"] = {"pt.soc_pct", bms_.soc_pct, "%", Timestamp::now()};
    m["pt.motor_torque_nm"] = {"pt.motor_torque_nm", motor_.torque_nm, "Nm", Timestamp::now()};
    m["pt.motor_power_kw"] = {"pt.motor_power_kw", motor_.power_kw, "kW", Timestamp::now()};
    m["pt.batt_temp_c"] = {"pt.batt_temp_c", bms_.temp_c, "C", Timestamp::now()};
    return m;
}

std::vector<std::string> PowertrainModule::capabilities() const {
    return {
        "Electric Motor Control",
        "Battery Management System (BMS)",
        "Traction Inverter",
        "Thermal Management",
        "Drive Mode Selection",
        "DC Fast Charging Interface"
    };
}

} // namespace powertrain
} // namespace sdv
