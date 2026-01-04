#pragma once

#include "../di/service_registry.hpp"
#include "../services/interfaces/i_logger.hpp"
#include <memory>

namespace sdl3cpp::controllers {

/**
 * @brief Main application controller.
 *
 * Orchestrates the main game loop and coordinates all subsystems.
 * Replaces Sdl3App::MainLoop() with clean service-based architecture.
 */
class ApplicationController {
public:
    explicit ApplicationController(di::ServiceRegistry& registry);
    ~ApplicationController();

    ApplicationController(const ApplicationController&) = delete;
    ApplicationController& operator=(const ApplicationController&) = delete;

    /**
     * @brief Run the main application loop.
     *
     * Processes events, updates game state, and renders frames until quit.
     */
    void Run();

private:
    void ProcessFrame(float deltaTime);
    void HandleEvents();

    di::ServiceRegistry& registry_;
    std::shared_ptr<services::ILogger> logger_;
    bool running_ = false;
};

}  // namespace sdl3cpp::controllers
