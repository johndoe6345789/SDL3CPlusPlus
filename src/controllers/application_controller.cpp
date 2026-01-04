#include "application_controller.hpp"
#include "../logging/logger.hpp"
#include "../services/interfaces/i_window_service.hpp"
#include "../services/interfaces/i_input_service.hpp"
#include "../events/event_bus.hpp"
#include "../events/event_types.hpp"

namespace sdl3cpp::controllers {

ApplicationController::ApplicationController(di::ServiceRegistry& registry)
    : registry_(registry), running_(false) {
    logging::TraceGuard trace;
}

ApplicationController::~ApplicationController() {
    logging::TraceGuard trace;
}

void ApplicationController::Run() {
    logging::TraceGuard trace;
    logging::Logger::GetInstance().Info("Application starting main loop");

    running_ = true;
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (running_) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        HandleEvents();
        ProcessFrame(deltaTime);
    }

    logging::Logger::GetInstance().Info("Application exiting main loop");
}

void ApplicationController::HandleEvents() {
    // Window service polls SDL events and publishes to event bus
    auto windowService = registry_.GetService<services::IWindowService>();
    if (windowService) {
        windowService->PollEvents();
    }

    // Check for quit events
    auto eventBus = registry_.GetService<events::EventBus>();
    if (eventBus) {
        // Process queued events
        eventBus->ProcessQueue();
    }

    // Check if window should close
    if (windowService && windowService->ShouldClose()) {
        running_ = false;
    }
}

void ApplicationController::ProcessFrame(float deltaTime) {
    logging::TraceGuard trace;

    // Render controller will handle rendering pipeline
    // Physics, scripts, audio updates happen here
    // This is a placeholder - full implementation in RenderController
}

}  // namespace sdl3cpp::controllers
