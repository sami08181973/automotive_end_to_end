#pragma once
/**
 * @file ee_architecture.hpp
 * @brief E/E Architecture – Distributed, Domain, Zonal, Centralized paradigms
 */

#include "sdv/common.hpp"
#include <string>
#include <vector>

namespace sdv {
namespace ee {

enum class EEParadigm {
    Distributed,   // Classic many-ECU
    Domain,        // Domain controllers
    Zonal,         // Zone controllers + central compute
    Centralized    // HPC central computer
};

enum class NetworkBus { CAN, CAN_FD, LIN, FlexRay, Ethernet_100BASE_T1, Ethernet_1000BASE_T1, PCIe };

struct ECUNode {
    std::string name;
    std::string role;
    ASIL        asil;
    bool        online;
    double      cpu_load_pct;
};

struct ZoneController {
    std::string name;
    std::string location;   // FrontLeft, FrontRight, RearLeft, RearRight, Cabin
    int         ecu_count;
    bool        online;
    NetworkBus  uplink;
};

struct CentralCompute {
    std::string name;
    int         core_count;
    double      tops_ai;       // AI TOPS
    double      ram_gb;
    bool        online;
    double      load_pct;
};

class EEArchitectureModule : public ArchitectureModule {
public:
    EEArchitectureModule();

    bool initialize() override;
    bool start() override;
    void tick(double dt_sec) override;
    void stop() override;
    void shutdown() override;
    SignalMap exportSignals() const override;
    std::vector<std::string> capabilities() const override;

    void setParadigm(EEParadigm paradigm);
    void migrateToZonal();
    void migrateToCentralized();
    int totalECUs() const;
    int totalZones() const;

    EEParadigm paradigm() const { return paradigm_; }
    const CentralCompute& hpc() const { return hpc_; }

private:
    void buildDistributed();
    void buildDomain();
    void buildZonal();
    void buildCentralized();

    EEParadigm                 paradigm_;
    std::vector<ECUNode>       ecus_;
    std::vector<ZoneController> zones_;
    std::vector<ECUNode>       domain_controllers_;
    CentralCompute             hpc_;
    std::vector<NetworkBus>    backbone_;
    double                     sim_time_;
};

} // namespace ee
} // namespace sdv
