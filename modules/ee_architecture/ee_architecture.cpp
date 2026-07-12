/**
 * @file ee_architecture.cpp
 * @brief E/E Architecture implementation – paradigm evolution demo
 */

#include "ee_architecture.hpp"

namespace sdv {
namespace ee {

EEArchitectureModule::EEArchitectureModule()
    : ArchitectureModule(ArchitectureDomain::EEArchitecture, "EEArchitecture", ASIL::D)
    , paradigm_(EEParadigm::Distributed)
    , sim_time_(0.0)
{
    hpc_ = {"", 0, 0.0, 0.0, false, 0.0};
}

bool EEArchitectureModule::initialize() {
    setStatus(SystemStatus::Initializing);
    buildDistributed();
    logEvent(domain_, "E/E topology loaded – starting from Distributed paradigm");
    setStatus(SystemStatus::Ready);
    return true;
}

bool EEArchitectureModule::start() {
    setStatus(SystemStatus::Active);
    // Demonstrate evolution path
    migrateToZonal();
    return true;
}

void EEArchitectureModule::tick(double dt_sec) {
    if (status_ != SystemStatus::Active) return;
    sim_time_ += dt_sec;

    for (auto& e : ecus_) {
        if (e.online) e.cpu_load_pct = 20.0 + 15.0 * std::sin(sim_time_ + e.cpu_load_pct);
        e.cpu_load_pct = clamp(e.cpu_load_pct, 5.0, 95.0);
    }
    if (hpc_.online) {
        hpc_.load_pct = clamp(40.0 + 20.0 * std::sin(sim_time_ * 0.5), 10.0, 90.0);
    }
}

void EEArchitectureModule::stop() {
    setStatus(SystemStatus::Ready);
}

void EEArchitectureModule::shutdown() {
    ecus_.clear();
    zones_.clear();
    hpc_.online = false;
    setStatus(SystemStatus::Offline);
}

void EEArchitectureModule::setParadigm(EEParadigm paradigm) {
    paradigm_ = paradigm;
    switch (paradigm) {
        case EEParadigm::Distributed:  buildDistributed();  break;
        case EEParadigm::Domain:       buildDomain();       break;
        case EEParadigm::Zonal:        buildZonal();        break;
        case EEParadigm::Centralized:  buildCentralized();  break;
    }
}

void EEArchitectureModule::migrateToZonal() {
    logEvent(domain_, "Migrating Distributed/Domain -> Zonal architecture...");
    buildZonal();
}

void EEArchitectureModule::migrateToCentralized() {
    logEvent(domain_, "Migrating Zonal -> Centralized HPC architecture...");
    buildCentralized();
}

int EEArchitectureModule::totalECUs() const {
    return static_cast<int>(ecus_.size() + domain_controllers_.size());
}

int EEArchitectureModule::totalZones() const {
    return static_cast<int>(zones_.size());
}

void EEArchitectureModule::buildDistributed() {
    paradigm_ = EEParadigm::Distributed;
    ecus_.clear();
    zones_.clear();
    domain_controllers_.clear();
    hpc_ = {"", 0, 0.0, 0.0, false, 0.0};
    backbone_ = {NetworkBus::CAN, NetworkBus::LIN};

    const char* names[] = {
        "ECM", "TCM", "ABS", "ESC", "BCM", "IC", "AC", "SRS",
        "TPMS", "EPS", "GW", "TCU", "HUD", "SCL", "PLG",
        "DoorFL", "DoorFR", "SeatDrv", "ParkAid", "RadarFL"
    };
    for (auto* n : names) {
        ecus_.push_back({n, "FunctionECU", ASIL::B, true, 30.0});
    }
    logEvent(domain_, "Distributed: " + std::to_string(ecus_.size())
             + " ECUs on CAN/LIN – high wiring complexity");
}

void EEArchitectureModule::buildDomain() {
    paradigm_ = EEParadigm::Domain;
    ecus_.clear();
    zones_.clear();
    backbone_ = {NetworkBus::CAN_FD, NetworkBus::Ethernet_100BASE_T1};

    domain_controllers_ = {
        {"PowertrainDC",  "DomainController", ASIL::C, true, 45.0},
        {"ChassisDC",     "DomainController", ASIL::D, true, 50.0},
        {"BodyDC",        "DomainController", ASIL::B, true, 35.0},
        {"CabinDC",       "DomainController", ASIL::B, true, 55.0},
        {"ADASDC",        "DomainController", ASIL::D, true, 70.0}
    };
    for (int i = 0; i < 12; ++i)
        ecus_.push_back({"SatECU_" + std::to_string(i), "Satellite", ASIL::A, true, 20.0});

    logEvent(domain_, "Domain: " + std::to_string(domain_controllers_.size())
             + " domain controllers + " + std::to_string(ecus_.size()) + " satellites");
}

void EEArchitectureModule::buildZonal() {
    paradigm_ = EEParadigm::Zonal;
    ecus_.clear();
    domain_controllers_.clear();
    backbone_ = {NetworkBus::Ethernet_1000BASE_T1, NetworkBus::CAN_FD};

    zones_ = {
        {"Zone_FL", "FrontLeft",  8, true, NetworkBus::Ethernet_1000BASE_T1},
        {"Zone_FR", "FrontRight", 8, true, NetworkBus::Ethernet_1000BASE_T1},
        {"Zone_RL", "RearLeft",   6, true, NetworkBus::Ethernet_1000BASE_T1},
        {"Zone_RR", "RearRight",  6, true, NetworkBus::Ethernet_1000BASE_T1},
        {"Zone_Cabin", "Cabin",   10, true, NetworkBus::Ethernet_1000BASE_T1}
    };

    hpc_ = {"CentralCompute_A", 12, 100.0, 32.0, true, 40.0};
    ecus_.push_back({"SmartActuator_1", "Actuator", ASIL::B, true, 15.0});
    ecus_.push_back({"SmartActuator_2", "Actuator", ASIL::B, true, 15.0});

    logEvent(domain_, "Zonal: " + std::to_string(zones_.size())
             + " zone controllers + central compute ("
             + std::to_string(static_cast<int>(hpc_.tops_ai)) + " TOPS)");
    logEvent(domain_, "Backbone: Automotive Ethernet 1000BASE-T1");
}

void EEArchitectureModule::buildCentralized() {
    paradigm_ = EEParadigm::Centralized;
    ecus_.clear();
    domain_controllers_.clear();
    zones_ = {
        {"Zone_FL", "FrontLeft",  4, true, NetworkBus::Ethernet_1000BASE_T1},
        {"Zone_FR", "FrontRight", 4, true, NetworkBus::Ethernet_1000BASE_T1},
        {"Zone_RL", "RearLeft",   3, true, NetworkBus::Ethernet_1000BASE_T1},
        {"Zone_RR", "RearRight",  3, true, NetworkBus::Ethernet_1000BASE_T1}
    };
    backbone_ = {NetworkBus::Ethernet_1000BASE_T1, NetworkBus::PCIe};
    hpc_ = {"VehicleHPC", 24, 500.0, 64.0, true, 35.0};

    logEvent(domain_, "Centralized: Vehicle HPC ("
             + std::to_string(hpc_.core_count) + " cores, "
             + std::to_string(static_cast<int>(hpc_.tops_ai)) + " TOPS AI, "
             + std::to_string(static_cast<int>(hpc_.ram_gb)) + " GB RAM)");
    logEvent(domain_, "Software-defined functions consolidated on HPC");
}

SignalMap EEArchitectureModule::exportSignals() const {
    SignalMap m;
    m["ee.ecu_count"] = {"ee.ecu_count", static_cast<double>(totalECUs()), "count", Timestamp::now()};
    m["ee.zone_count"] = {"ee.zone_count", static_cast<double>(totalZones()), "count", Timestamp::now()};
    m["ee.hpc_load_pct"] = {"ee.hpc_load_pct", hpc_.load_pct, "%", Timestamp::now()};
    m["ee.hpc_tops"] = {"ee.hpc_tops", hpc_.tops_ai, "TOPS", Timestamp::now()};
    return m;
}

std::vector<std::string> EEArchitectureModule::capabilities() const {
    return {
        "Distributed ECU Topology",
        "Domain Controller Architecture",
        "Zonal Architecture",
        "Centralized HPC Computing",
        "Automotive Ethernet Backbone",
        "Paradigm Migration Path"
    };
}

} // namespace ee
} // namespace sdv
