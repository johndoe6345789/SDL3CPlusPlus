#pragma once

#include "../di/service_registry.hpp"
#include "../services/interfaces/i_logger.hpp"

namespace sdl3cpp::controllers {

/**
 * @brief Lifecycle controller.
 *
 * Manages initialization and shutdown sequencing of all services.
 * Ensures proper dependency order and resource cleanup.
 */
class LifecycleController {
public:
    explicit LifecycleController(di::ServiceRegistry& registry);
    ~LifecycleController();

    LifecycleController(const LifecycleController&) = delete;
    LifecycleController& operator=(const LifecycleController&) = delete;

    /**
     * @brief Initialize all services in correct order.
     */
    void InitializeAll();

    /**
     * @brief Shutdown all services in reverse order.
     */
    void ShutdownAll() noexcept;

private:
    di::ServiceRegistry& registry_;
    std::shared_ptr<services::ILogger> logger_;
};

}  // namespace sdl3cpp::controllers
