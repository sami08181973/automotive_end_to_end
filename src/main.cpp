/**
 * ============================================================================
 * @file        main.cpp
 * @brief       Automotive Architectures – End-to-End Software-Defined Vehicle (SDV)
 *              Integration Demo (runnable console application)
 *
 * @details     This program constructs and exercises eleven architecture modules
 *              that together represent a modern vehicle platform:
 *
 *                1. Infotainment      – IVI, digital cockpit, nav, voice, apps
 *                2. Connectivity      – 4G/5G, V2X, OTA, telematics, cloud
 *                3. ADAS & Autonomous – perception, fusion, decision, automation
 *                4. Body & Comfort    – HVAC, lighting, seats, keyless, BCM
 *                5. Powertrain        – motor/BMS/inverter/thermal (BEV focus)
 *                6. Chassis & Safety  – ABS, ESC, TPMS, airbags, steer/brake
 *                7. E/E Architecture  – Distributed / Domain / Zonal / Central
 *                8. Security          – secure boot, crypto, gateway, ISO 21434
 *                9. Functional        – features, requirements, safety, DTCs
 *               10. Software Platform – AUTOSAR, middleware, OS, SOA, lifecycle
 *               11. Vehicle Data      – edge-to-cloud, analytics, governance, twin
 *               12. Virtual ECU       – CPU/GPU/DSP/ARM/Cloud HPC backends
 *               13. HPC / CUDA        – realtime optimized compute suite
 *
 *              Cross-cutting platform services demonstrated here:
 *                - Health monitoring / deadline supervision
 *                - CRC-16 payload integrity (CAN / SOME/IP style)
 *                - Hypervisor + QNX + Adaptive AUTOSAR runtime labels
 *                - Virtual ECU on Colab/GCP/AWS/Azure/Docker/K8s (no local HW)
 *
 *              Design intent: educational / architectural simulation that is
 *              fully runnable — not production ECU firmware.
 *
 * @author      Muhammad Samiullah (Founder & CTO)
 * @company     Embedded AI Design Labs Pvt Ltd
 * @email       muhammadsami@embedailabs.com
 * @copyright   Copyright (c) 2026 Embedded AI Design Labs Pvt Ltd.
 *              All rights reserved.
 * @watermark   SDV End-to-End Architectures – Embedded AI Design Labs Pvt Ltd
 * ============================================================================
 */

#include "sdv/common.hpp"
#include "sdv/copyright.hpp"
#include "sdv/health_monitor.hpp"

/* ---- Module headers (one architecture per folder under modules/) ---- */
#include "../modules/infotainment/infotainment.hpp"
#include "../modules/connectivity/connectivity.hpp"
#include "../modules/adas/adas.hpp"
#include "../modules/body_comfort/body_comfort.hpp"
#include "../modules/powertrain/powertrain.hpp"
#include "../modules/chassis_safety/chassis_safety.hpp"
#include "../modules/ee_architecture/ee_architecture.hpp"
#include "../modules/security/security.hpp"
#include "../modules/functional/functional.hpp"
#include "../modules/software_platform/software_platform.hpp"
#include "../modules/vehicle_data/vehicle_data.hpp"
#include "../modules/virtual_ecu/virtual_ecu.hpp"
#include "../modules/hpc/hpc.hpp"

#include <iostream>
#include <vector>
#include <iomanip>
#include <cstring>

using namespace sdv;

/**
 * @brief Print the primary product banner (company, author, copyright, watermark).
 *
 * Shown once at process start so console runs carry clear ownership attribution.
 */
static void printBanner() {
    using namespace sdv::legal;
    std::cout << R"(
╔══════════════════════════════════════════════════════════════════════════════╗
║                                                                              ║
║   AUTOMOTIVE ARCHITECTURES – END-TO-END SOFTWARE-DEFINED VEHICLE (SDV)       ║
║                                                                              ║
║   Infotainment | Connectivity | ADAS | Body | Powertrain | Chassis           ║
║   E/E | Security | Functional | Software | Vehicle Data                      ║
║                                                                              ║
╠══════════════════════════════════════════════════════════════════════════════╣
║   Author  : Muhammad Samiullah (Founder & CTO)                               ║
║   Company : Embedded AI Design Labs Pvt Ltd                                  ║
║   Email   : muhammadsami@embedailabs.com                                     ║
║   )" << COPYRIGHT_NOTICE << R"(
║   Watermark: )" << WATERMARK << R"(
╚══════════════════════════════════════════════════════════════════════════════╝
)" << "\n";
}

/**
 * @brief Print a short narrative of what this demo will execute.
 */
static void printDescription() {
    std::cout
        << "DESCRIPTION\n"
        << "-----------\n"
        << "  This runnable demo initializes eleven SDV architecture modules, performs\n"
        << "  secure boot, starts cross-domain scenarios (voice nav, OTA+signature,\n"
        << "  gateway firewall, ADAS decisions, E/E migration to centralized HPC),\n"
        << "  aggregates signals into an edge-to-cloud / digital-twin pipeline, and\n"
        << "  exercises health monitoring with CRC-16 on CAN/SOMEIP-style payloads.\n"
        << "\n"
        << "  Presentation docs (diagrams, DOORS requirements, ASPICE/ASIL):\n"
        << "    open docs/index.html\n"
        << "\n";
}

/**
 * @brief Dump ASIL-tagged capability list for one module.
 */
static void printModuleCapabilities(const IArchitectureModule& m) {
    std::cout << "  [" << asilName(m.safetyLevel()) << "] " << m.name() << "\n";
    for (const auto& c : m.capabilities()) {
        std::cout << "      • " << c << "\n";
    }
}

/**
 * @brief Application entry – orchestrates the full SDV ecosystem simulation.
 * @return 0 on success, non-zero if initialization or secure boot fails
 */
int main() {
    /* ---- Ownership banner + narrative ---- */
    printBanner();
    printDescription();

    /*
     * Construct one instance per architecture domain.
     * Order of construction is independent; start order matters for security.
     */
    infotainment::InfotainmentModule   ivi;     // Driver experience / HMI
    connectivity::ConnectivityModule   conn;    // Cellular, V2X, OTA, cloud
    adas::ADASModule                   adas_m;  // Perception → decision
    body::BodyComfortModule            body;    // HVAC / BCM / keyless
    powertrain::PowertrainModule       pt;      // BEV motor + BMS
    chassis::ChassisSafetyModule       chassis; // ABS/ESC/SRS
    ee::EEArchitectureModule           ee;      // E/E paradigm evolution
    security::SecurityModule           sec;     // Trust root
    functional::FunctionalModule       func;    // Reqs / features / DTCs
    software::SoftwarePlatformModule   sw;      // AUTOSAR / SOA / lifecycle
    data::VehicleDataModule            vdata;   // Edge-cloud / twin
    vecu::VirtualECU                   vecu;    // Virtual ECU / cloud HW
    hpc::HpcModule                     hpcmod;  // HPC / CUDA realtime

    /* Platform health monitor (hypervisor / QNX / Adaptive partitions) */
    platform::HealthMonitor health;

    /* Pointer table for uniform initialize / tick / shutdown loops */
    std::vector<IArchitectureModule*> modules = {
        &sec, &ee, &sw, &func, &pt, &chassis, &adas_m, &body, &ivi, &conn, &vdata, &vecu, &hpcmod
    };

    /* ================================================================== */
    /* PHASE 1 – Initialize every architecture                            */
    /* ================================================================== */
    Logger::instance().separator("PHASE 1: INITIALIZE ALL ARCHITECTURES");

    for (auto* m : modules) {
        // Each module transitions Offline → Initializing → Ready
        if (!m->initialize()) {
            logError(m->domain(), "Initialization FAILED");
            return 1;
        }
    }

    /* Register supervised runtimes (ASPICE-style evidence of monitoring) */
    health.registerEntity("SecureGateway",     "AdaptiveAUTOSAR", 1.0);
    health.registerEntity("ADAS_Partition",    "QNX_Safety",      0.5);
    health.registerEntity("IVI_GuestVM",       "Hypervisor_VM",   2.0);
    health.registerEntity("BodyCAN_Gateway",   "ClassicAUTOSAR",  1.0);
    health.registerEntity("CloudAgent",        "Linux",           3.0);

    Logger::instance().separator("CAPABILITY MAP");
    for (auto* m : modules) {
        printModuleCapabilities(*m);
    }

    /* ================================================================== */
    /* PHASE 2 – Secure boot first, then start remaining domains          */
    /* ================================================================== */
    Logger::instance().separator("PHASE 2: START (Security boot first)");

    // Chain of trust must pass before vehicle functions go Active
    if (!sec.start()) {
        logError(ArchitectureDomain::Security,
                 "Secure boot failed – aborting vehicle start");
        return 1;
    }
    health.heartbeat("SecureGateway", 0.0);

    for (auto* m : modules) {
        if (m == &sec) continue; // already started
        m->start();
    }
    health.heartbeat("ADAS_Partition", 0.0);
    health.heartbeat("IVI_GuestVM", 0.0);
    health.heartbeat("BodyCAN_Gateway", 0.0);
    health.heartbeat("CloudAgent", 0.0);

    /* CRC protect a synthetic SOME/IP / CAN payload */
    {
        const char* payload = "SOMEIP:VehicleSpeed:27.8";
        auto* bytes = reinterpret_cast<const std::uint8_t*>(payload);
        std::size_t len = std::strlen(payload);
        std::uint16_t crc = health.protectPayload("BodyCAN_Gateway", bytes, len);
        // Verify immediately (integrity check demo)
        (void)health.verifyPayload("BodyCAN_Gateway", bytes, len, crc);
    }

    /* ================================================================== */
    /* PHASE 3 – Cross-domain scenario (ecosystem integration)            */
    /* ================================================================== */
    Logger::instance().separator("PHASE 3: CROSS-DOMAIN SCENARIO");

    // Infotainment: voice → navigation + app launch
    ivi.voiceCommand("Navigate to City Center");
    ivi.launchApp("MusicStreaming");

    // Body comfort: seat heat + fan
    body.setSeatHeat(body::SeatZone::Driver, 2);
    body.setFanSpeed(3);

    // Connectivity + Security: telematics, OTA, signature, gateway ACL
    conn.sendTelematics("IGNITION_ON", "vin=SDV-DEMO-001");
    conn.startOTA("ADAS_Perception", "2.2.0", 128);
    sec.verifyOTAPackage("ADAS_Perception", "SIG_A1B2C3D4");
    sec.filterMessage("TCU", "Cloud", "MQTT");            // allow
    sec.filterMessage("External", "CAN_Internal", "Any"); // deny + threat

    // Powertrain + Chassis + ADAS interaction
    pt.setDriveMode(powertrain::DriveMode::Sport);
    pt.requestTorque(120.0);
    chassis.setBrakePedal(0.0);
    chassis.setSteeringAngle(5.0);
    chassis.simulateLowTirePressure(2);
    adas_m.injectObject({adas::ObjectClass::Vehicle, 22.0, 0.5, 0.94, 20.0});

    // E/E evolution toward centralized HPC
    ee.migrateToCentralized();

    // Functional diagnostics
    func.raiseDTC("U0428", "Invalid data received from steering angle sensor");
    func.runDiagnostic("FEAT-AEB");

    // Virtual ECU: exercise CPU/GPU/ARM/DSP/Cloud use-cases (no local HW required)
    Logger::instance().separator("PHASE 3b: VIRTUAL ECU / CLOUD HW USE-CASES");
    auto vecu_jobs = vecu.runAllUsecases();
    logEvent(ArchitectureDomain::VirtualECU,
             "Completed " + std::to_string(vecu_jobs.size()) + " Virtual ECU jobs");

    Logger::instance().separator("PHASE 3c: HPC REALTIME CUDA/CPU USE-CASES");
    auto hpc_jobs = hpcmod.runSuite();
    logEvent(ArchitectureDomain::HPC,
             "Completed " + std::to_string(hpc_jobs.size()) + " HPC bench rows");

    /* ================================================================== */
    /* PHASE 4 – Timed simulation loop                                    */
    /* ================================================================== */
    Logger::instance().separator("PHASE 4: SIMULATION LOOP (5 seconds)");

    const double dt = 0.5;        // tick period [s]
    const double duration = 5.0;  // total simulated time [s]
    for (double t = 0.0; t < duration; t += dt) {
        // Advance each architecture one discrete step
        for (auto* m : modules) {
            m->tick(dt);
        }

        // Health supervision + heartbeats
        health.heartbeat("SecureGateway", t);
        health.heartbeat("ADAS_Partition", t);
        health.heartbeat("IVI_GuestVM", t);
        health.heartbeat("BodyCAN_Gateway", t);
        health.heartbeat("CloudAgent", t);
        health.tick(t);

        // Aggregate exported signals into vehicle-data edge buffer
        SignalMap batch;
        for (auto* m : modules) {
            auto sigs = m->exportSignals();
            batch.insert(sigs.begin(), sigs.end());
        }
        vdata.ingestBatch(batch);

        sleepMs(50); // pace console output for readability
    }

    /* ================================================================== */
    /* PHASE 5 – Digital twin + analytics                                 */
    /* ================================================================== */
    Logger::instance().separator("PHASE 5: DIGITAL TWIN & ANALYTICS");
    vdata.runEdgePipeline();
    vdata.syncDigitalTwin();
    auto insights = vdata.runAnalytics();
    (void)insights;

    /* ================================================================== */
    /* PHASE 6 – Snapshot                                                 */
    /* ================================================================== */
    Logger::instance().separator("PHASE 6: SYSTEM SNAPSHOT");

    std::cout << std::fixed << std::setprecision(1);
    std::cout << "\n  Module Status Summary:\n";
    for (auto* m : modules) {
        std::cout << "    " << std::setw(28) << std::left << m->name()
                  << "  " << statusName(m->status())
                  << "  [" << asilName(m->safetyLevel()) << "]\n";
    }

    std::cout << "\n  Key Telemetry:\n";
    std::cout << "    SoC:            " << pt.battery().soc_pct << " %\n";
    std::cout << "    Motor Torque:   " << pt.motor().torque_nm << " Nm\n";
    std::cout << "    Ego Speed:      "
              << adas_m.exportSignals().at("adas.ego_speed_mps").value << " m/s\n";
    std::cout << "    Cabin Temp:     " << body.hvac().cabin_temp_c << " C\n";
    std::cout << "    Nav Remaining:  " << ivi.navigation().remaining_km << " km\n";
    std::cout << "    OTA Progress:   " << conn.ota().progress_pct << " %\n";
    std::cout << "    E/E Zones:      " << ee.totalZones() << "\n";
    std::cout << "    HPC TOPS:       " << ee.hpc().tops_ai << "\n";
    std::cout << "    Apps Running:   " << sw.runningApps() << "\n";
    std::cout << "    Features Active:" << func.activeFeatures() << "\n";
    std::cout << "    Twin Fidelity:  " << vdata.twin().fidelity_pct << " %\n";
    std::cout << "    ISO 21434:      "
              << (sec.checkISO21434Compliance() ? "COMPLIANT" : "GAP") << "\n";
    std::cout << "    Data Governance:"
              << (vdata.checkGovernance() ? "OK" : "FAIL") << "\n";
    std::cout << "    TPMS:           "
              << (chassis.checkTPMS() ? "OK" : "WARNING") << "\n";
    std::cout << "    Health OK:      " << health.healthyCount()
              << " / " << health.entities().size() << " entities\n";
    std::cout << "    vECU jobs:      " << vecu.exportSignals().at("vecu.jobs").value << "\n";
    std::cout << "    vECU TOPS:      " << vecu.hw().gpu_tops << "\n";
    std::cout << "    HPC speedup:    " << hpcmod.bestSpeedup() << " x\n";
    std::cout << "    HPC avg lat:    " << hpcmod.lastAvgLatencyMs() << " ms\n";
    std::cout << "    HPC CUDA:       " << (hpcmod.usingCuda() ? "YES" : "NO") << "\n";

    /* ================================================================== */
    /* PHASE 7 – Ordered shutdown                                         */
    /* ================================================================== */
    Logger::instance().separator("PHASE 7: SHUTDOWN");

    // Reverse order: data plane down before trust root
    for (auto it = modules.rbegin(); it != modules.rend(); ++it) {
        (*it)->stop();
        (*it)->shutdown();
    }

    using namespace sdv::legal;
    std::cout << "\n"
              << "╔══════════════════════════════════════════════════════════════════════════════╗\n"
              << "║  Demo complete. Multiple architectures operated as one intelligent ecosystem.║\n"
              << "║  Open docs/index.html for diagrams, DOORS requirements, ASPICE & ASIL.       ║\n"
              << "║                                                                              ║\n"
              << "║  " << AUTHOR_NAME << " (" << AUTHOR_TITLE << ")                               \n"
              << "║  " << COMPANY_NAME << "\n"
              << "║  " << AUTHOR_EMAIL << "\n"
              << "║  " << COPYRIGHT_NOTICE << "\n"
              << "║  Watermark: " << WATERMARK << "\n"
              << "╚══════════════════════════════════════════════════════════════════════════════╝\n\n";

    return 0;
}
