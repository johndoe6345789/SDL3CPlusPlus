#pragma once

#include <filesystem>
#include <memory>
#include <SDL3/SDL.h>
#include "di/service_registry.hpp"
#include "controllers/lifecycle_controller.hpp"
#include "controllers/application_controller.hpp"
#include "services/interfaces/i_application_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_crash_recovery_service.hpp"

namespace sdl3cpp::app {

/**
 * @brief Modern service-based application.
 *
 * Replaces the monolithic Sdl3App with a clean dependency injection architecture.
 */
class ServiceBasedApp : public services::IApplicationService {
public:
    explicit ServiceBasedApp(const std::filesystem::path& scriptPath);
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

    std::filesystem::path scriptPath_;
    di::ServiceRegistry registry_;
    std::unique_ptr<controllers::LifecycleController> lifecycleController_;
    std::unique_ptr<controllers::ApplicationController> applicationController_;
    std::shared_ptr<services::ILogger> logger_;
    std::shared_ptr<services::ICrashRecoveryService> crashRecoveryService_;
};

}  // namespace sdl3cpp::app
