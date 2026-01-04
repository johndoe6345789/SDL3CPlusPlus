#pragma once

#include <filesystem>
#include <memory>
#include <SDL3/SDL.h>
#include "di/service_registry.hpp"
#include "controllers/lifecycle_controller.hpp"
#include "controllers/application_controller.hpp"
#include "services/interfaces/i_logger.hpp"

namespace sdl3cpp::app {

/**
 * @brief Modern service-based application.
 *
 * Replaces the monolithic Sdl3App with a clean dependency injection architecture.
 */
class ServiceBasedApp {
public:
    explicit ServiceBasedApp(const std::filesystem::path& scriptPath);
    ~ServiceBasedApp();

    ServiceBasedApp(const ServiceBasedApp&) = delete;
    ServiceBasedApp& operator=(const ServiceBasedApp&) = delete;

    /**
     * @brief Run the application main loop.
     */
    void Run();

private:
    void RegisterServices();
    void SetupSDL();

    std::filesystem::path scriptPath_;
    di::ServiceRegistry registry_;
    std::unique_ptr<controllers::LifecycleController> lifecycleController_;
    std::unique_ptr<controllers::ApplicationController> applicationController_;
    std::shared_ptr<services::ILogger> logger_;
};

}  // namespace sdl3cpp::app