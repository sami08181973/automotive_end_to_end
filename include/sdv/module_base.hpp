#pragma once
/**
 * @file module_base.hpp
 * @brief Abstract base for all SDV architecture modules
 */

#include "types.hpp"
#include "logger.hpp"
#include <string>
#include <vector>

namespace sdv {

class IArchitectureModule {
public:
    virtual ~IArchitectureModule() = default;

    virtual ArchitectureDomain domain() const = 0;
    virtual const char* name() const = 0;
    virtual ASIL safetyLevel() const = 0;

    virtual bool initialize() = 0;
    virtual bool start() = 0;
    virtual void tick(double dt_sec) = 0;
    virtual void stop() = 0;
    virtual void shutdown() = 0;

    virtual SystemStatus status() const = 0;
    virtual SignalMap exportSignals() const = 0;
    virtual std::vector<std::string> capabilities() const = 0;
};

class ArchitectureModule : public IArchitectureModule {
public:
    explicit ArchitectureModule(ArchitectureDomain domain, const char* name, ASIL asil)
        : domain_(domain), name_(name), asil_(asil), status_(SystemStatus::Offline) {}

    ArchitectureDomain domain() const override { return domain_; }
    const char* name() const override { return name_; }
    ASIL safetyLevel() const override { return asil_; }
    SystemStatus status() const override { return status_; }

protected:
    ArchitectureDomain domain_;
    const char*        name_;
    ASIL               asil_;
    SystemStatus       status_;

    void setStatus(SystemStatus s) {
        status_ = s;
        logInfo(domain_, std::string(name_) + " -> " + statusName(s));
    }
};

} // namespace sdv
