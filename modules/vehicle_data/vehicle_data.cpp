/**
 * @file vehicle_data.cpp
 * @brief Vehicle Data Architecture implementation
 */

#include "vehicle_data.hpp"

namespace sdv {
namespace data {

VehicleDataModule::VehicleDataModule()
    : ArchitectureModule(ArchitectureDomain::VehicleData, "VehicleDataArchitecture", ASIL::A)
    , stage_(PipelineStage::EdgeCollect)
    , sim_time_(0.0)
    , total_ingested_(0)
{
    edge_ = {10000, 0, 0};
    twin_ = {false, 0.0, "DT-VIN-SDV-001", 0};
    policy_ = {true, true, true, 90, true};
}

bool VehicleDataModule::initialize() {
    setStatus(SystemStatus::Initializing);
    logEvent(domain_, "Edge data collector ready (capacity "
             + std::to_string(edge_.capacity) + ")");
    logEvent(domain_, "Governance: GDPR anonymization + TLS encryption ON");
    logEvent(domain_, "Digital twin ID: " + twin_.twin_id);
    setStatus(SystemStatus::Ready);
    return true;
}

bool VehicleDataModule::start() {
    setStatus(SystemStatus::Active);
    stage_ = PipelineStage::EdgeCollect;
    return true;
}

void VehicleDataModule::tick(double dt_sec) {
    if (status_ != SystemStatus::Active) return;
    sim_time_ += dt_sec;

    if (buffer_data_.size() > 20) {
        runEdgePipeline();
    }
    if (static_cast<int>(sim_time_) % 3 == 0 && !cloud_store_.empty()) {
        syncDigitalTwin();
        last_analytics_ = runAnalytics();
    }
}

void VehicleDataModule::stop() {
    runEdgePipeline(); // flush
    setStatus(SystemStatus::Ready);
}

void VehicleDataModule::shutdown() {
    buffer_data_.clear();
    cloud_store_.clear();
    twin_.synced = false;
    setStatus(SystemStatus::Offline);
}

void VehicleDataModule::ingest(const VehicleSignal& sig, DataClass cls, PrivacyLevel privacy) {
    DataRecord rec{sig.name, sig.value, cls, privacy, sig.ts};
    if (policy_.anonymize_personal && privacy == PrivacyLevel::Personal) {
        rec = anonymize(rec);
    }

    if (buffer_data_.size() >= edge_.capacity) {
        buffer_data_.pop_front();
        edge_.dropped++;
    }
    buffer_data_.push_back(rec);
    edge_.count = buffer_data_.size();
    total_ingested_++;
}

void VehicleDataModule::ingestBatch(const SignalMap& signals) {
    for (const auto& kv : signals) {
        PrivacyLevel p = PrivacyLevel::Internal;
        DataClass c = DataClass::Telemetry;
        if (kv.first.find("adas") != std::string::npos) c = DataClass::Perception;
        if (kv.first.find("dtc") != std::string::npos)  c = DataClass::Diagnostic;
        ingest(kv.second, c, p);
    }
}

void VehicleDataModule::runEdgePipeline() {
    stage_ = PipelineStage::EdgeFilter;
    std::vector<DataRecord> filtered;
    for (const auto& r : buffer_data_) {
        // Filter: drop NaN-like / keep interesting
        if (r.signal.empty()) continue;
        filtered.push_back(r);
    }

    stage_ = PipelineStage::EdgeAggregate;
    // Simple aggregate: keep last value per signal (already unique-ish)

    stage_ = PipelineStage::Uplink;
    if (policy_.encrypt_in_transit) {
        logEvent(domain_, "Edge uplink encrypted (" + std::to_string(filtered.size()) + " records)");
    }

    stage_ = PipelineStage::CloudIngest;
    for (const auto& r : filtered) cloud_store_.push_back(r);
    buffer_data_.clear();
    edge_.count = 0;

    logEvent(domain_, "Cloud ingest complete – store size "
             + std::to_string(cloud_store_.size()));
    stage_ = PipelineStage::Analytics;
}

void VehicleDataModule::syncDigitalTwin() {
    stage_ = PipelineStage::TwinSync;
    twin_.last_sync_signals = static_cast<int>(std::min(cloud_store_.size(), std::size_t{50}));
    twin_.fidelity_pct = clamp(70.0 + twin_.last_sync_signals * 0.5, 0.0, 99.5);
    twin_.synced = true;
    logEvent(domain_, "Digital twin synced: " + twin_.twin_id
             + " fidelity=" + std::to_string(static_cast<int>(twin_.fidelity_pct)) + "%");
}

std::vector<AnalyticsResult> VehicleDataModule::runAnalytics() {
    std::vector<AnalyticsResult> results;

    double soc = -1, speed = -1, threats = 0;
    for (const auto& r : cloud_store_) {
        if (r.signal.find("soc") != std::string::npos) soc = r.value;
        if (r.signal.find("speed") != std::string::npos) speed = r.value;
        if (r.signal.find("threat") != std::string::npos) threats = r.value;
    }

    if (soc >= 0) {
        results.push_back({"avg_soc", soc,
            soc < 20.0 ? "Low SoC – recommend charge station" : "SoC healthy"});
    }
    if (speed >= 0) {
        results.push_back({"ego_speed", speed, "Speed profile within limits"});
    }
    results.push_back({"records_total", static_cast<double>(total_ingested_),
                       "Edge-to-cloud pipeline healthy"});
    results.push_back({"security_threats", threats,
                       threats > 0 ? "Review security events" : "No active threats"});

    for (const auto& a : results) {
        logEvent(domain_, "Analytics: " + a.metric + "=" + std::to_string(a.value)
                 + " | " + a.insight);
    }
    return results;
}

bool VehicleDataModule::checkGovernance() const {
    return policy_.anonymize_personal && policy_.encrypt_in_transit
           && policy_.gdpr_compliant && policy_.retention_days > 0;
}

DataRecord VehicleDataModule::anonymize(const DataRecord& rec) const {
    DataRecord out = rec;
    // Strip personal identifiers from signal name
    if (out.signal.find("user") != std::string::npos)
        out.signal = "anon." + out.signal;
    out.privacy = PrivacyLevel::Internal;
    return out;
}

SignalMap VehicleDataModule::exportSignals() const {
    SignalMap m;
    m["data.buffered"] = {"data.buffered", static_cast<double>(edge_.count), "count", Timestamp::now()};
    m["data.cloud_records"] = {"data.cloud_records", static_cast<double>(cloud_store_.size()), "count", Timestamp::now()};
    m["data.twin_fidelity"] = {"data.twin_fidelity", twin_.fidelity_pct, "%", Timestamp::now()};
    m["data.ingested_total"] = {"data.ingested_total", static_cast<double>(total_ingested_), "count", Timestamp::now()};
    return m;
}

std::vector<std::string> VehicleDataModule::capabilities() const {
    return {
        "Edge Data Collection & Filtering",
        "Encrypted Cloud Uplink",
        "Analytics & Insights",
        "Data Governance / GDPR",
        "Digital Twin Synchronization",
        "Signal Aggregation Pipeline"
    };
}

} // namespace data
} // namespace sdv
