#pragma once

#include "probe_types.hpp"
#include <vector>

namespace sdl3cpp::services {

/**
 * @brief Probe service interface for structured diagnostics.
 */
class IProbeService {
public:
    virtual ~IProbeService() = default;

    /**
     * @brief Record a probe report.
     *
     * @param report Report data
     */
    virtual void Report(const ProbeReport& report) = 0;

    /**
     * @brief Drain all queued reports.
     *
     * @return Collected reports (clears internal storage)
     */
    virtual std::vector<ProbeReport> DrainReports() = 0;

    /**
     * @brief Clear all queued reports.
     */
    virtual void ClearReports() = 0;
};

}  // namespace sdl3cpp::services
