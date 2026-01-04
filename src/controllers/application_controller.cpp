#include "application_controller.hpp"
#include "render_controller.hpp"
#include "../services/interfaces/i_logger.hpp"
#include "../services/interfaces/i_window_service.hpp"
#include "../services/interfaces/i_input_service.hpp"
#include "../services/interfaces/i_physics_service.hpp"
#include "../services/interfaces/i_scene_service.hpp"
#include "../events/event_bus.hpp"
#include "../events/event_types.hpp"
#include <chrono>

namespace sdl3cpp::controllers {

ApplicationController::ApplicationController(di::ServiceRegistry& registry)
    : registry_(registry), logger_(registry.GetService<services::ILogger>()), running_(false) {
    logger_->Trace("ApplicationController", "ApplicationController", "", "Created");
}

ApplicationController::~ApplicationController() {
    logger_->Trace("ApplicationController", "~ApplicationController", "", "Destroyed");
}

void ApplicationController::Run() {
    logger_->Trace("ApplicationController", "Run", "", "Entering");
    logger_->Info("ApplicationController::Run: ApplicationController::Run starting");
    logger_->Info("ApplicationController::Run: Application starting main loop");

    running_ = true;
    auto lastTime = std::chrono::high_resolution_clock::now();
    auto startTime = lastTime;
    const auto timeout = std::chrono::seconds(5);  // Exit after 5 seconds for testing

    while (running_) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        
        // Exit after timeout for headless testing
        if (currentTime - startTime > timeout) {
            logger_->Info("ApplicationController::Run: Application timeout reached, exiting");
            running_ = false;
            break;
        }
        
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        HandleEvents();
        ProcessFrame(deltaTime);
    }

    logger_->Info("ApplicationController::Run: Application exiting main loop");
    logger_->Trace("ApplicationController", "Run", "", "Exiting");
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
    logger_->Trace("ApplicationController", "ProcessFrame", "deltaTime=" + std::to_string(deltaTime), "Entering");

    // Reset input frame state
    auto inputService = registry_.GetService<services::IInputService>();
    if (inputService) {
        inputService->ResetFrameState();
    }

    // Update physics
    auto physicsService = registry_.GetService<services::IPhysicsService>();
    if (physicsService) {
        physicsService->StepSimulation(deltaTime);
    }

    // Update scene
    auto sceneService = registry_.GetService<services::ISceneService>();
    if (sceneService) {
        sceneService->UpdateScene(deltaTime);
    }

    // Update GUI input to script service
    if (inputService) {
        inputService->UpdateGuiInput();
    }

    // Render frame
    // Temporarily disabled for testing
    // auto renderController = std::make_unique<RenderController>(registry_);
    // renderController->RenderFrame(static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
    //     std::chrono::high_resolution_clock::now().time_since_epoch()).count()) / 1000.0f);

    logger_->Trace("ApplicationController", "ProcessFrame", "", "Exiting");
}

}  // namespace sdl3cpp::controllers
