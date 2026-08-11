#include "di/service_registry.hpp"
#include <algorithm>

namespace sdl3cpp::di {

void ServiceRegistry::InitializeAll() {
    if (initialized_) {
        throw std::runtime_error("Services already initialized");
    }

    // Call all initialization functions in registration order
    for (const auto& initFunc : initFunctions_) {
        initFunc();
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
