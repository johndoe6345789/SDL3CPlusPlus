#include "service_registry.hpp"
#include "../logging/logger.hpp"
#include <algorithm>

namespace sdl3cpp::di {

void ServiceRegistry::InitializeAll() {
    if (initialized_) {
        throw std::runtime_error("Services already initialized");
    }

    // Call all initialization functions in registration order
    for (size_t i = 0; i < initFunctions_.size(); ++i) {
        logging::Logger::GetInstance().Info("Initializing service " + std::to_string(i + 1) + " of " + std::to_string(initFunctions_.size()));
        initFunctions_[i]();
    }

    initialized_ = true;
}

void ServiceRegistry::ShutdownAll() noexcept {
    if (!initialized_) {
        return;  // Nothing to shutdown
    }

    // Call all shutdown functions in reverse registration order
    for (auto it = shutdownFunctions_.rbegin(); it != shutdownFunctions_.rend(); ++it) {
        try {
            (*it)();
        } catch (...) {
            // Shutdown methods must be noexcept, but just in case...
            // Swallow exceptions to ensure all services get shutdown
        }
    }

    initialized_ = false;
}

}  // namespace sdl3cpp::di
