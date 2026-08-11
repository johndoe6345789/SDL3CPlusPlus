#include "services/interfaces/diagnostics/probe_service.hpp"

namespace sdl3cpp::services::impl {

ProbeService::ProbeService(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("ProbeService", "Constructor", "Entry");
    }
}

std::string ProbeService::GetServiceId() const {
    return "probe.default";
}

}  // namespace sdl3cpp::services::impl
