#pragma once
/**
 * @file vehicle_data.hpp
 * @brief Vehicle Data Architecture – edge-to-cloud, analytics, governance, digital twin
 */

#include "sdv/common.hpp"
#include <string>
#include <vector>
#include <deque>

namespace sdv {
namespace data {

enum class DataClass    { Telemetry, Event, Diagnostic, Perception, UserPref, DigitalTwin };
enum class PrivacyLevel { Public, Internal, Personal, Restricted };
enum class PipelineStage{ EdgeCollect, EdgeFilter, EdgeAggregate, Uplink, CloudIngest,
                          Analytics, TwinSync, Archive };

struct DataRecord {
    std::string  signal;
    double       value;
    DataClass    cls;
    PrivacyLevel privacy;
    Timestamp    ts;
};

struct EdgeBuffer {
    std::size_t capacity;
    std::size_t count;
    std::size_t dropped;
};

struct AnalyticsResult {
    std::string metric;
    double      value;
    std::string insight;
};

struct DigitalTwinState {
    bool   synced;
    double fidelity_pct;
    std::string twin_id;
    int    last_sync_signals;
};

struct GovernancePolicy {
    bool anonymize_personal;
    bool encrypt_in_transit;
    bool retain_diagnostics_days;
    int  retention_days;
    bool gdpr_compliant;
};

class VehicleDataModule : public ArchitectureModule {
public:
    VehicleDataModule();

    bool initialize() override;
    bool start() override;
    void tick(double dt_sec) override;
    void stop() override;
    void shutdown() override;
    SignalMap exportSignals() const override;
    std::vector<std::string> capabilities() const override;

    void ingest(const VehicleSignal& sig, DataClass cls, PrivacyLevel privacy);
    void ingestBatch(const SignalMap& signals);
    void runEdgePipeline();
    void syncDigitalTwin();
    std::vector<AnalyticsResult> runAnalytics();
    bool checkGovernance() const;
    std::size_t bufferedCount() const { return buffer_data_.size(); }

    const DigitalTwinState& twin() const { return twin_; }
    const EdgeBuffer& edgeBuffer() const { return edge_; }

private:
    DataRecord anonymize(const DataRecord& rec) const;

    EdgeBuffer                 edge_;
    std::deque<DataRecord>     buffer_data_;
    std::vector<DataRecord>    cloud_store_;
    DigitalTwinState           twin_;
    GovernancePolicy           policy_;
    std::vector<AnalyticsResult> last_analytics_;
    PipelineStage              stage_;
    double                     sim_time_;
    std::size_t                total_ingested_;
};

} // namespace data
} // namespace sdv
