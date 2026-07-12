#pragma once
/**
 * @file logger.hpp
 * @brief Simple console logger for SDV architecture demos
 */

#include "types.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <mutex>

namespace sdv {

class Logger {
public:
    static Logger& instance() {
        static Logger log;
        return log;
    }

    void info(ArchitectureDomain domain, const std::string& msg) {
        write("INFO ", domain, msg);
    }

    void warn(ArchitectureDomain domain, const std::string& msg) {
        write("WARN ", domain, msg);
    }

    void error(ArchitectureDomain domain, const std::string& msg) {
        write("ERROR", domain, msg);
    }

    void event(ArchitectureDomain domain, const std::string& msg) {
        write("EVENT", domain, msg);
    }

    void separator(const std::string& title = "") {
        std::lock_guard<std::mutex> lock(mtx_);
        std::cout << "\n";
        if (title.empty()) {
            std::cout << "================================================================\n";
        } else {
            std::cout << "==== " << title << " "
                      << std::string(60 - title.size(), '=') << "\n";
        }
    }

private:
    std::mutex mtx_;

    void write(const char* level, ArchitectureDomain domain, const std::string& msg) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto ts = Timestamp::now();
        std::cout << "[" << ts.epoch_ms << "] "
                  << "[" << level << "] "
                  << "[" << std::setw(20) << std::left << domainName(domain) << "] "
                  << msg << "\n";
    }
};

inline void logInfo(ArchitectureDomain d, const std::string& m)  { Logger::instance().info(d, m); }
inline void logWarn(ArchitectureDomain d, const std::string& m)  { Logger::instance().warn(d, m); }
inline void logError(ArchitectureDomain d, const std::string& m) { Logger::instance().error(d, m); }
inline void logEvent(ArchitectureDomain d, const std::string& m) { Logger::instance().event(d, m); }

} // namespace sdv
