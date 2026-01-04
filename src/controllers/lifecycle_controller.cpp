#include "lifecycle_controller.hpp"
#include "../services/interfaces/i_logger.hpp"

namespace sdl3cpp::controllers {

LifecycleController::LifecycleController(di::ServiceRegistry& registry)
    : registry_(registry), logger_(registry.GetService<services::ILogger>()) {
    logger_->Trace("LifecycleController", "LifecycleController", "", "Created");
}

LifecycleController::~LifecycleController() {
    logger_->Trace("LifecycleController", "~LifecycleController", "", "Destroyed");
}

void LifecycleController::InitializeAll() {
    logger_->Trace("LifecycleController", "InitializeAll", "", "Entering");
    logger_->Info("LifecycleController::InitializeAll: Initializing all services");

    // ServiceRegistry handles initialization order based on dependencies
    registry_.InitializeAll();

    logger_->Info("LifecycleController::InitializeAll: All services initialized");
    logger_->Trace("LifecycleController", "InitializeAll", "", "Exiting");
}

void LifecycleController::ShutdownAll() noexcept {
    logger_->Trace("LifecycleController", "ShutdownAll", "", "Entering");
    logger_->Info("LifecycleController::ShutdownAll: Shutting down all services");

    // ServiceRegistry handles shutdown in reverse dependency order
    registry_.ShutdownAll();

    logger_->Info("LifecycleController::ShutdownAll: All services shutdown");
    logger_->Trace("LifecycleController::ShutdownAll: Exiting");
}

}  // namespace sdl3cpp::controllers
