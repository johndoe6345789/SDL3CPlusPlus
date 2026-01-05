#include "lifecycle_service.hpp"

namespace sdl3cpp::services::impl {

LifecycleService::LifecycleService(di::ServiceRegistry& registry, std::shared_ptr<ILogger> logger)
    : registry_(registry), logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("LifecycleService", "LifecycleService", "registry=provided");
    }
}

void LifecycleService::InitializeAll() {
    if (logger_) {
        logger_->Trace("LifecycleService", "InitializeAll", "", "Entering");
        logger_->Info("LifecycleService::InitializeAll: Initializing all services");
    }

    registry_.InitializeAll();

    if (logger_) {
        logger_->Info("LifecycleService::InitializeAll: All services initialized");
        logger_->Trace("LifecycleService", "InitializeAll", "", "Exiting");
    }
}

void LifecycleService::ShutdownAll() noexcept {
    if (logger_) {
        logger_->Trace("LifecycleService", "ShutdownAll", "", "Entering");
        logger_->Info("LifecycleService::ShutdownAll: Shutting down all services");
    }

    registry_.ShutdownAll();

    if (logger_) {
        logger_->Info("LifecycleService::ShutdownAll: All services shutdown");
        logger_->Trace("LifecycleService", "ShutdownAll", "", "Exiting");
    }
}

}  // namespace sdl3cpp::services::impl
