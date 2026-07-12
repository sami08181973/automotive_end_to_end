#pragma once
/**
 * ============================================================================
 * @file        virtual_ecu.hpp
 * @brief       Virtual ECU with pluggable hardware backends (CPU / GPU / DSP / ARM / Cloud)
 *
 * Hardware-dependent paths are compiled when SDV_HAS_CUDA is defined; otherwise
 * a software simulation backend runs so demos work without local accelerators.
 * Cloud targets (Colab, GCP, AWS, Azure) run the same binary inside containers.
 *
 * @author      Muhammad Samiullah (Founder & CTO)
 * @company     Embedded AI Design Labs Pvt Ltd
 * @email       muhammadsami@embedailabs.com
 * @copyright   Copyright (c) 2026 Embedded AI Design Labs Pvt Ltd. All rights reserved.
 * ============================================================================
 */

#include "sdv/common.hpp"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace sdv {
namespace vecu {

/** Physical / virtual compute fabric available to an ECU image. */
enum class HwBackend {
    CPU_Sim,       /**< Always available ? pure software */
    NVIDIA_GPU,    /**< CUDA when SDV_HAS_CUDA; else emulated TOPS */
    ARM_Cortex,    /**< Cross-compile / QEMU / Graviton / Ampere */
    DSP_Accel,     /**< TI C7x / Hexagon style DSP (simulated) */
    Cloud_HPC      /**< GCP/AWS/Azure/Colab container node */
};

enum class CloudProvider {
    None,
    LocalDocker,
    GoogleColab,
    GCP,
    AWS,
    Azure,
    Kubernetes
};

struct HwCapabilities {
    HwBackend backend;
    std::string device_name;
    int    cpu_cores;
    double gpu_tops;       /**< AI TOPS (real or emulated) */
    int    dsp_macs_g;     /**< GMAC/s equivalent */
    bool   cuda_available;
    bool   cloud_hosted;
    CloudProvider provider;
};

struct WorkloadResult {
    std::string name;
    double      latency_ms;
    double      throughput_ops;
    bool        used_accelerator;
    std::string backend_note;
};

/**
 * @class IHwBackend
 * @brief Hardware-dependent interface ? real CUDA/ARM/DSP OR software fallback.
 */
class IHwBackend {
public:
    virtual ~IHwBackend() = default;
    virtual HwCapabilities caps() const = 0;
    virtual bool initialize() = 0;
    virtual WorkloadResult runPerceptionKernel(int frames) = 0;
    virtual WorkloadResult runSignalProcessing(int samples) = 0;
    virtual WorkloadResult runControlLoop(int ticks) = 0;
    virtual void shutdown() = 0;
};

/** Factory: prefers CUDA if compiled in; else CPU sim; cloud metadata optional. */
std::unique_ptr<IHwBackend> createBackend(HwBackend prefer,
                                          CloudProvider cloud = CloudProvider::None);

/**
 * @class VirtualECU
 * @brief Cloud-storable ECU image that binds domain software to a hw backend.
 *
 * "Virtual ECU stored in cloud" = container image + this runtime binding to
 * free/shared accelerators (Colab GPU, GCP free tier, AWS free tier, Azure).
 */
class VirtualECU : public ArchitectureModule {
public:
    VirtualECU();

    bool initialize() override;
    bool start() override;
    void tick(double dt_sec) override;
    void stop() override;
    void shutdown() override;
    SignalMap exportSignals() const override;
    std::vector<std::string> capabilities() const override;

    /** Bind preferred hardware + optional cloud provider label. */
    bool attachHardware(HwBackend backend, CloudProvider cloud = CloudProvider::None);

    /** Run ADAS-like perception use-case on attached backend. */
    WorkloadResult usecasePerception(int frames = 30);

    /** Run DSP-style filtering use-case. */
    WorkloadResult usecaseDspFilter(int samples = 4096);

    /** Run chassis control loop use-case (ARM-style deterministic). */
    WorkloadResult usecaseControl(int ticks = 100);

    /** Enumerate all backends (real + emulated) for demos. */
    std::vector<WorkloadResult> runAllUsecases();

    const HwCapabilities& hw() const { return caps_; }
    CloudProvider cloud() const { return caps_.provider; }

private:
    std::unique_ptr<IHwBackend> backend_;
    HwCapabilities              caps_;
    int                         jobs_done_;
    double                      sim_time_;
};

inline const char* backendName(HwBackend b) {
    switch (b) {
        case HwBackend::CPU_Sim:    return "CPU_Sim";
        case HwBackend::NVIDIA_GPU: return "NVIDIA_GPU";
        case HwBackend::ARM_Cortex: return "ARM_Cortex";
        case HwBackend::DSP_Accel:  return "DSP_Accel";
        case HwBackend::Cloud_HPC:  return "Cloud_HPC";
        default: return "Unknown";
    }
}

inline const char* cloudName(CloudProvider c) {
    switch (c) {
        case CloudProvider::None:         return "None";
        case CloudProvider::LocalDocker:  return "LocalDocker";
        case CloudProvider::GoogleColab:  return "GoogleColab";
        case CloudProvider::GCP:          return "GCP";
        case CloudProvider::AWS:          return "AWS";
        case CloudProvider::Azure:        return "Azure";
        case CloudProvider::Kubernetes:   return "Kubernetes";
        default: return "Unknown";
    }
}

} // namespace vecu
} // namespace sdv
