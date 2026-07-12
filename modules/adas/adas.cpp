/**
 * @file adas.cpp
 * @brief ADAS & Autonomous Architecture implementation
 */

#include "adas.hpp"

namespace sdv {
namespace adas {

ADASModule::ADASModule()
    : ArchitectureModule(ArchitectureDomain::ADAS, "ADASAutonomousArchitecture", ASIL::D)
    , level_(AutonomyLevel::L2Plus)
    , ego_speed_mps_(0.0)
    , sim_time_(0.0)
{
    decision_ = {DrivingMode::Manual, 0.0, 0.0, false, "standby"};
    fusion_ = {{}, 0, true};
}

bool ADASModule::initialize() {
    setStatus(SystemStatus::Initializing);

    sensors_ = {
        {SensorType::Camera,     "CAM_FRONT",  true, 30.0, 150.0},
        {SensorType::Camera,     "CAM_REAR",   true, 30.0, 80.0},
        {SensorType::Camera,     "CAM_SIDE_L", true, 30.0, 60.0},
        {SensorType::Camera,     "CAM_SIDE_R", true, 30.0, 60.0},
        {SensorType::Radar,      "RAD_FRONT",  true, 20.0, 200.0},
        {SensorType::Radar,      "RAD_CORNER", true, 20.0, 80.0},
        {SensorType::Lidar,      "LIDAR_TOP",  true, 10.0, 200.0},
        {SensorType::Ultrasonic, "USS_FRONT",  true, 40.0, 5.0},
        {SensorType::IMU,        "IMU_1",      true, 100.0, 0.0},
        {SensorType::GNSS,       "GNSS_1",     true, 10.0, 0.0}
    };

    logEvent(domain_, "Perception stack online – " + std::to_string(sensors_.size()) + " sensors");
    logEvent(domain_, "Sensor fusion + planning pipeline ready (ASIL-D)");
    setStatus(SystemStatus::Ready);
    return true;
}

bool ADASModule::start() {
    setStatus(SystemStatus::Active);
    setDrivingMode(DrivingMode::HighwayPilot);
    ego_speed_mps_ = 27.8; // ~100 km/h
    injectObject({ObjectClass::Vehicle, 40.0, 0.0, 0.92, 25.0});
    injectObject({ObjectClass::Lane, 0.0, -1.75, 0.95, 0.0});
    injectObject({ObjectClass::Lane, 0.0, 1.75, 0.95, 0.0});
    return true;
}

void ADASModule::tick(double dt_sec) {
    if (status_ != SystemStatus::Active) return;
    sim_time_ += dt_sec;

    runPerception();
    runFusion();
    runDecision();

    // Simulate ego following
    if (!decision_.brake_request) {
        ego_speed_mps_ = lerp(ego_speed_mps_, decision_.target_speed_mps, 0.1);
    } else {
        ego_speed_mps_ = std::max(0.0, ego_speed_mps_ - 3.0 * dt_sec);
    }
}

void ADASModule::stop() {
    decision_.mode = DrivingMode::Manual;
    setStatus(SystemStatus::Ready);
}

void ADASModule::shutdown() {
    sensors_.clear();
    raw_detections_.clear();
    setStatus(SystemStatus::Offline);
}

void ADASModule::setAutonomyLevel(AutonomyLevel level) {
    level_ = level;
    const char* names[] = {"L0", "L1", "L2", "L2+", "L3", "L4", "L5"};
    logEvent(domain_, std::string("Autonomy level -> ") + names[static_cast<int>(level)]);
}

void ADASModule::setDrivingMode(DrivingMode mode) {
    decision_.mode = mode;
    const char* names[] = {"Manual", "ACC", "LKA", "HighwayPilot", "AutomatedParking", "Autonomous"};
    logEvent(domain_, std::string("Driving mode -> ") + names[static_cast<int>(mode)]);
}

void ADASModule::injectObject(const DetectedObject& obj) {
    raw_detections_.push_back(obj);
}

void ADASModule::clearObjects() {
    raw_detections_.clear();
}

void ADASModule::runPerception() {
    // Sensors produce detections (already injected / simulated)
    int online = 0;
    for (const auto& s : sensors_) if (s.online) ++online;
    if (online < 6) {
        logWarn(domain_, "Perception degraded – sensor count low");
        setStatus(SystemStatus::Degraded);
    }
}

void ADASModule::runFusion() {
    fusion_.objects = raw_detections_;
    fusion_.lane_confidence = 90;
    fusion_.free_space_ok = true;

    for (const auto& o : fusion_.objects) {
        if (o.cls == ObjectClass::Pedestrian && o.x_m < 15.0) {
            fusion_.free_space_ok = false;
        }
    }
}

void ADASModule::runDecision() {
    decision_.brake_request = false;
    decision_.steering_angle_deg = 0.0;
    decision_.target_speed_mps = 27.8;
    decision_.reason = "cruise";

    for (const auto& o : fusion_.objects) {
        if (o.cls == ObjectClass::Vehicle && o.x_m < 30.0) {
            double gap = o.x_m;
            if (gap < 15.0) {
                decision_.brake_request = true;
                decision_.target_speed_mps = o.velocity_mps * 0.8;
                decision_.reason = "AEB/ACC cut-in: lead vehicle close";
                logEvent(domain_, "Decision: " + decision_.reason);
            } else {
                decision_.target_speed_mps = std::min(decision_.target_speed_mps, o.velocity_mps);
                decision_.reason = "ACC following lead vehicle";
            }
        }
    }

    if (!fusion_.free_space_ok) {
        decision_.brake_request = true;
        decision_.target_speed_mps = 0.0;
        decision_.reason = "Emergency stop – vulnerable road user";
        logEvent(domain_, "Decision: " + decision_.reason);
    }
}

SignalMap ADASModule::exportSignals() const {
    SignalMap m;
    m["adas.ego_speed_mps"] = {"adas.ego_speed_mps", ego_speed_mps_, "m/s", Timestamp::now()};
    m["adas.objects"] = {"adas.objects", static_cast<double>(fusion_.objects.size()), "count", Timestamp::now()};
    m["adas.lane_conf"] = {"adas.lane_conf", static_cast<double>(fusion_.lane_confidence), "%", Timestamp::now()};
    m["adas.target_speed"] = {"adas.target_speed", decision_.target_speed_mps, "m/s", Timestamp::now()};
    return m;
}

std::vector<std::string> ADASModule::capabilities() const {
    return {
        "Multi-sensor Perception (Camera/Radar/LiDAR/USS)",
        "Sensor Fusion",
        "Path Planning & Decision Making",
        "ACC / LKA / Highway Pilot",
        "AEB / Blind Spot / Parking Assist",
        "Autonomy Levels L0–L4"
    };
}

} // namespace adas
} // namespace sdv
