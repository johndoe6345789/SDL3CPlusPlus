#pragma once

#include <memory>
#include <SDL3/SDL.h>
#include "di/service_registry.hpp"
#include "services/interfaces/i_application_loop_service.hpp"
#include "services/interfaces/i_lifecycle_service.hpp"
#include "services/interfaces/i_render_coordinator_service.hpp"
#include "services/interfaces/i_application_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_crash_recovery_service.hpp"
#include "services/interfaces/config_types.hpp"

namespace sdl3cpp::app {

/**
 * @brief Modern service-based application.
 *
 * Replaces the monolithic Sdl3App with a clean dependency injection architecture.
 */
class ServiceBasedApp : public services::IApplicationService {
public:
    explicit ServiceBasedApp(services::RuntimeConfig runtimeConfig, services::LogLevel logLevel);
    ~ServiceBasedApp();

    ServiceBasedApp(const ServiceBasedApp&) = delete;
    ServiceBasedApp& operator=(const ServiceBasedApp&) = delete;

    /**
     * @brief Run the application main loop.
     */
    void Run() override;

    /**
     * @brief Configure the logger service.
     *
     * @param level The logging level
     * @param enableConsole Whether to enable console output
     * @param outputFile Path to the log file (optional)
     */
    void ConfigureLogging(services::LogLevel level, bool enableConsole, const std::string& outputFile = "") override;

    /**
     * @brief Get the logger service for external configuration.
     *
     * @return Shared pointer to the logger service
     */
    std::shared_ptr<services::ILogger> GetLogger() const override { return logger_; }

private:
    void RegisterServices();
    void SetupSDL();

    services::RuntimeConfig runtimeConfig_;
    di::ServiceRegistry registry_;
    std::shared_ptr<services::ILifecycleService> lifecycleService_;
    std::shared_ptr<services::IApplicationLoopService> applicationLoopService_;
    std::shared_ptr<services::IRenderCoordinatorService> renderCoordinatorService_;
    std::shared_ptr<services::ILogger> logger_;
    std::shared_ptr<services::ICrashRecoveryService> crashRecoveryService_;
};

}  // namespace sdl3cpp::app
