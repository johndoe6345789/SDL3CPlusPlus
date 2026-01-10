#include "ecs_service.hpp"

namespace sdl3cpp::services::impl {

EcsService::EcsService(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("EcsService", "EcsService");
    }
}

void EcsService::Shutdown() noexcept {
    if (logger_) {
        logger_->Trace("EcsService", "Shutdown");
    }
    registry_.clear();
}

}  // namespace sdl3cpp::services::impl
