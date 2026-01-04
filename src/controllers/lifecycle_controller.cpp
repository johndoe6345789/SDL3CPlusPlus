#include "lifecycle_controller.hpp"
#include "../logging/logger.hpp"

namespace sdl3cpp::controllers {

LifecycleController::LifecycleController(di::ServiceRegistry& registry)
    : registry_(registry) {
    logging::TraceGuard trace;
}

LifecycleController::~LifecycleController() {
    logging::TraceGuard trace;
}

void LifecycleController::InitializeAll() {
    logging::TraceGuard trace;
    logging::Logger::GetInstance().Info("Initializing all services");

    // ServiceRegistry handles initialization order based on dependencies
    registry_.InitializeAll();

    logging::Logger::GetInstance().Info("All services initialized");
}

void LifecycleController::ShutdownAll() noexcept {
    logging::TraceGuard trace;
    logging::Logger::GetInstance().Info("Shutting down all services");

    // ServiceRegistry handles shutdown in reverse dependency order
    registry_.ShutdownAll();

    logging::Logger::GetInstance().Info("All services shutdown");
}

}  // namespace sdl3cpp::controllers
