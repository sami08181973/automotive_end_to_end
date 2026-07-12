#pragma once
/**
 * ============================================================================
 * @file        health_monitor.hpp
 * @brief       Health monitoring, CRC-16/CCITT, and platform integrity checks
 *
 * Demonstrates automotive platform concerns used across SDV domains:
 *  - Periodic health monitoring (alive / deadline supervision style)
 *  - CRC-16 integrity over CAN / SOME/IP payload buffers
 *  - References Adaptive AUTOSAR, hypervisor partitions, QNX safety OS
 *
 * @author      Muhammad Samiullah (Founder & CTO)
 * @company     Embedded AI Design Labs Pvt Ltd
 * @email       muhammadsami@embedailabs.com
 * @copyright   Copyright (c) 2026 Embedded AI Design Labs Pvt Ltd. All rights reserved.
 * ============================================================================
 */

#include "types.hpp"
#include "logger.hpp"
#include "copyright.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace sdv {
namespace platform {

/**
 * @enum HealthState
 * @brief Coarse health of a supervised software/hardware entity.
 */
enum class HealthState {
    Unknown,   /**< Not yet observed */
    Healthy,   /**< Within deadline / CRC OK */
    Degraded,  /**< Recoverable anomaly */
    Failed     /**< Supervision failure */
};

/**
 * @struct SupervisedEntity
 * @brief One monitored channel (e.g. ADAS partition, CAN gateway).
 */
struct SupervisedEntity {
    std::string  id;            /**< Logical name, e.g. "ADAS_Partition" */
    std::string  runtime;       /**< QNX / Linux / AdaptiveAUTOSAR / HypervisorVM */
    HealthState  state;
    double       last_alive_s;  /**< Simulation time of last heartbeat */
    double       deadline_s;    /**< Max allowed gap between heartbeats */
    std::uint16_t last_crc;     /**< Last computed CRC-16 */
};

/**
 * @brief CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF) – common in autosar COM.
 * @param data  Bytes to protect (e.g. CAN frame payload or SOME/IP body)
 * @param len   Number of bytes
 * @return 16-bit CRC
 *
 * Line-level note: used for end-to-end protection concepts (E2E profile lite demo).
 */
inline std::uint16_t crc16_ccitt(const std::uint8_t* data, std::size_t len) {
    std::uint16_t crc = 0xFFFFu;
    for (std::size_t i = 0; i < len; ++i) {
        crc ^= static_cast<std::uint16_t>(data[i]) << 8;
        for (int b = 0; b < 8; ++b) {
            if (crc & 0x8000u)
                crc = static_cast<std::uint16_t>((crc << 1) ^ 0x1021u);
            else
                crc = static_cast<std::uint16_t>(crc << 1);
        }
    }
    return crc;
}

/**
 * @class HealthMonitor
 * @brief Lightweight supervisor for multi-OS / hypervisor SDV platforms.
 *
 * Models concepts aligned with:
 *  - ASPICE SWE.3/SWE.4 verification evidence (demo)
 *  - ISO 26262 safety mechanisms (alive counters / CRC)
 *  - Adaptive AUTOSAR Execution Management health
 *  - QNX Neutrino + Type-1 hypervisor guest partitions
 */
class HealthMonitor {
public:
    HealthMonitor() = default;

    /** Register a supervised runtime entity. */
    void registerEntity(const std::string& id,
                        const std::string& runtime,
                        double deadline_s = 1.0) {
        entities_.push_back({id, runtime, HealthState::Unknown, 0.0, deadline_s, 0});
        logEvent(ArchitectureDomain::SoftwarePlatform,
                 "HealthMonitor registered: " + id + " on " + runtime);
    }

    /** Heartbeat / alive indication from an entity. */
    void heartbeat(const std::string& id, double sim_time_s) {
        for (auto& e : entities_) {
            if (e.id == id) {
                e.last_alive_s = sim_time_s;
                e.state = HealthState::Healthy;
                return;
            }
        }
    }

    /**
     * Protect a buffer with CRC-16 and store result on the entity.
     * @return computed CRC
     */
    std::uint16_t protectPayload(const std::string& id,
                                 const std::uint8_t* data,
                                 std::size_t len) {
        std::uint16_t crc = crc16_ccitt(data, len);
        for (auto& e : entities_) {
            if (e.id == id) {
                e.last_crc = crc;
                logEvent(ArchitectureDomain::SoftwarePlatform,
                         "CRC-16 OK for " + id + " = 0x" + toHex(crc));
                break;
            }
        }
        return crc;
    }

    /** Verify CRC matches expected value. */
    bool verifyPayload(const std::string& id,
                       const std::uint8_t* data,
                       std::size_t len,
                       std::uint16_t expected) {
        std::uint16_t crc = crc16_ccitt(data, len);
        bool ok = (crc == expected);
        if (!ok) {
            for (auto& e : entities_) {
                if (e.id == id) e.state = HealthState::Degraded;
            }
            logWarn(ArchitectureDomain::SoftwarePlatform,
                    "CRC mismatch on " + id);
        }
        return ok;
    }

    /** Deadline supervision tick. */
    void tick(double sim_time_s) {
        for (auto& e : entities_) {
            if (e.state == HealthState::Unknown) continue;
            double gap = sim_time_s - e.last_alive_s;
            if (gap > e.deadline_s) {
                e.state = HealthState::Failed;
                logWarn(ArchitectureDomain::SoftwarePlatform,
                        "Deadline miss: " + e.id + " (>" +
                        std::to_string(e.deadline_s) + "s)");
            }
        }
    }

    int healthyCount() const {
        int n = 0;
        for (const auto& e : entities_)
            if (e.state == HealthState::Healthy) ++n;
        return n;
    }

    const std::vector<SupervisedEntity>& entities() const { return entities_; }

private:
    std::vector<SupervisedEntity> entities_;

    static std::string toHex(std::uint16_t v) {
        const char* hex = "0123456789ABCDEF";
        std::string s(4, '0');
        s[0] = hex[(v >> 12) & 0xF];
        s[1] = hex[(v >> 8) & 0xF];
        s[2] = hex[(v >> 4) & 0xF];
        s[3] = hex[v & 0xF];
        return s;
    }
};

} // namespace platform
} // namespace sdv
