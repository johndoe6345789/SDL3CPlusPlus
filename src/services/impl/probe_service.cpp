#include "probe_service.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

ProbeService::ProbeService(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("ProbeService", "ProbeService", "initialized=true");
    }
}

void ProbeService::Report(const ProbeReport& report) {
    reports_.push_back(report);
    LogReport(report);
}

std::vector<ProbeReport> ProbeService::DrainReports() {
    std::vector<ProbeReport> drained = std::move(reports_);
    reports_.clear();
    return drained;
}

void ProbeService::ClearReports() {
    reports_.clear();
}

std::string ProbeService::SeverityName(ProbeSeverity severity) const {
    switch (severity) {
        case ProbeSeverity::Info:
            return "info";
        case ProbeSeverity::Warn:
            return "warn";
        case ProbeSeverity::Error:
            return "error";
        case ProbeSeverity::Fatal:
            return "fatal";
        default:
            return "unknown";
    }
}

void ProbeService::LogReport(const ProbeReport& report) const {
    if (!logger_) {
        return;
    }
    const std::string summary = "severity=" + SeverityName(report.severity) +
        ", code=" + report.code +
        ", jsonPath=" + report.jsonPath +
        ", renderPass=" + report.renderPass +
        ", resourceId=" + report.resourceId;

    if (report.severity == ProbeSeverity::Warn) {
        logger_->Warn("ProbeService::Report: " + report.message + " (" + summary + ")");
    } else if (report.severity == ProbeSeverity::Error || report.severity == ProbeSeverity::Fatal) {
        logger_->Error("ProbeService::Report: " + report.message + " (" + summary + ")");
    } else {
        logger_->Info("ProbeService::Report: " + report.message + " (" + summary + ")");
    }
}

}  // namespace sdl3cpp::services::impl
