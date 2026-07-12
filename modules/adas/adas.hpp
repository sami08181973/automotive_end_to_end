#pragma once
/**
 * @file adas.hpp
 * @brief ADAS & Autonomous Architecture – perception, fusion, decision, automated driving
 */

#include "sdv/common.hpp"
#include <string>
#include <vector>

namespace sdv {
namespace adas {

enum class AutonomyLevel { L0, L1, L2, L2Plus, L3, L4, L5 };
enum class SensorType    { Camera, Radar, Lidar, Ultrasonic, IMU, GNSS };
enum class ObjectClass   { Unknown, Pedestrian, Cyclist, Vehicle, Obstacle, Lane, TrafficSign };
enum class DrivingMode   { Manual, ACC, LKA, HighwayPilot, AutomatedParking, Autonomous };

struct Sensor {
    SensorType  type;
    std::string id;
    bool        online;
    double      fps;
    double      range_m;
};

struct DetectedObject {
    ObjectClass cls;
    double      x_m, y_m;
    double      confidence;
    double      velocity_mps;
};

struct FusionResult {
    std::vector<DetectedObject> objects;
    int   lane_confidence;
    bool  free_space_ok;
};

struct DecisionOutput {
    DrivingMode mode;
    double      target_speed_mps;
    double      steering_angle_deg;
    bool        brake_request;
    std::string reason;
};

class ADASModule : public ArchitectureModule {
public:
    ADASModule();

    bool initialize() override;
    bool start() override;
    void tick(double dt_sec) override;
    void stop() override;
    void shutdown() override;
    SignalMap exportSignals() const override;
    std::vector<std::string> capabilities() const override;

    void setAutonomyLevel(AutonomyLevel level);
    void setDrivingMode(DrivingMode mode);
    void injectObject(const DetectedObject& obj);
    void clearObjects();

    AutonomyLevel autonomyLevel() const { return level_; }
    DrivingMode drivingMode() const { return decision_.mode; }
    const FusionResult& fusion() const { return fusion_; }
    const DecisionOutput& decision() const { return decision_; }

private:
    void runPerception();
    void runFusion();
    void runDecision();

    AutonomyLevel             level_;
    std::vector<Sensor>       sensors_;
    std::vector<DetectedObject> raw_detections_;
    FusionResult              fusion_;
    DecisionOutput            decision_;
    double                    ego_speed_mps_;
    double                    sim_time_;
};

} // namespace adas
} // namespace sdv
