#pragma once

#include <memory>
#include <string>
#include <SDL3/SDL.h>
#include "di/service_registry.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/config_types.hpp"
#include "services/interfaces/workflow_context.hpp"

namespace sdl3cpp::app {

/**
 * @brief Minimal service-based application (greenfield implementation).
 *
 * Stub-only: registers logger service. No services - everything is workflow steps.
 */
class ServiceBasedApp {
public:
    explicit ServiceBasedApp(services::RuntimeConfig runtimeConfig,
                           services::LogLevel logLevel,
                           const std::string& bootstrapPackage = "bootstrap_mac",
                           const std::string& gamePackage = "seed");
    ~ServiceBasedApp() = default;

    ServiceBasedApp(const ServiceBasedApp&) = delete;
    ServiceBasedApp& operator=(const ServiceBasedApp&) = delete;

    /**
     * @brief Run the application (stub - no-op).
     */
    void Run();

    /**
     * @brief Configure the logger service.
     */
    void ConfigureLogging(services::LogLevel level, bool enableConsole, const std::string& outputFile = "");

    /**
     * @brief Get the logger service.
     */
    std::shared_ptr<services::ILogger> GetLogger() const { return logger_; }

private:
    services::RuntimeConfig runtimeConfig_;
    std::string bootstrapPackage_;
    std::string gamePackage_;
    di::ServiceRegistry registry_;
    std::shared_ptr<services::ILogger> logger_;
};

}  // namespace sdl3cpp::app
