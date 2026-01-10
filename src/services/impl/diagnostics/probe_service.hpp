#pragma once

#include "services/interfaces/i_probe_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include <memory>
#include <vector>

namespace sdl3cpp::services::impl {

class ProbeService : public IProbeService {
public:
    explicit ProbeService(std::shared_ptr<ILogger> logger);

    void Report(const ProbeReport& report) override;
    std::vector<ProbeReport> DrainReports() override;
    void ClearReports() override;

private:
    std::string SeverityName(ProbeSeverity severity) const;
    void LogReport(const ProbeReport& report) const;

    std::shared_ptr<ILogger> logger_;
    std::vector<ProbeReport> reports_{};
};

}  // namespace sdl3cpp::services::impl
