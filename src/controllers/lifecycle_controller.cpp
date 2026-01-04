#include "lifecycle_controller.hpp"
#include "../logging/logger.hpp"

namespace sdl3cpp::controllers {

LifecycleController::LifecycleController(di::ServiceRegistry& registry)
    : registry_(registry) {
    logging::Logger::GetInstance().Trace("LifecycleController::LifecycleController: Created");
}

LifecycleController::~LifecycleController() {
    logging::Logger::GetInstance().Trace("LifecycleController::~LifecycleController: Destroyed");
}

void LifecycleController::InitializeAll() {
    logging::Logger::GetInstance().Trace("LifecycleController::InitializeAll: Entering");
    logging::Logger::GetInstance().Info("LifecycleController::InitializeAll: Initializing all services");

    // ServiceRegistry handles initialization order based on dependencies
    registry_.InitializeAll();

    logging::Logger::GetInstance().Info("LifecycleController::InitializeAll: All services initialized");
    logging::Logger::GetInstance().Trace("LifecycleController::InitializeAll: Exiting");
}

void LifecycleController::ShutdownAll() noexcept {
    logging::Logger::GetInstance().Trace("LifecycleController::ShutdownAll: Entering");
    logging::Logger::GetInstance().Info("LifecycleController::ShutdownAll: Shutting down all services");

    // ServiceRegistry handles shutdown in reverse dependency order
    registry_.ShutdownAll();

    logging::Logger::GetInstance().Info("LifecycleController::ShutdownAll: All services shutdown");
    logging::Logger::GetInstance().Trace("LifecycleController::ShutdownAll: Exiting");
}

}  // namespace sdl3cpp::controllers
