#include "application_loop_service.hpp"

#include <chrono>

namespace sdl3cpp::services::impl {

ApplicationLoopService::ApplicationLoopService(std::shared_ptr<ILogger> logger,
                                               std::shared_ptr<IWindowService> windowService,
                                               std::shared_ptr<events::IEventBus> eventBus,
                                               std::shared_ptr<IInputService> inputService,
                                               std::shared_ptr<IPhysicsService> physicsService,
                                               std::shared_ptr<ISceneService> sceneService,
                                               std::shared_ptr<IRenderCoordinatorService> renderCoordinatorService,
                                               std::shared_ptr<IAudioService> audioService)
    : logger_(std::move(logger)),
      windowService_(std::move(windowService)),
      eventBus_(std::move(eventBus)),
      inputService_(std::move(inputService)),
      physicsService_(std::move(physicsService)),
      sceneService_(std::move(sceneService)),
      renderCoordinatorService_(std::move(renderCoordinatorService)),
      audioService_(std::move(audioService)) {
    if (logger_) {
        logger_->Trace("ApplicationLoopService", "ApplicationLoopService",
                       "windowService=" + std::string(windowService_ ? "set" : "null") +
                       ", eventBus=" + std::string(eventBus_ ? "set" : "null") +
                       ", inputService=" + std::string(inputService_ ? "set" : "null") +
                       ", physicsService=" + std::string(physicsService_ ? "set" : "null") +
                       ", sceneService=" + std::string(sceneService_ ? "set" : "null") +
                       ", renderCoordinatorService=" + std::string(renderCoordinatorService_ ? "set" : "null") +
                       ", audioService=" + std::string(audioService_ ? "set" : "null"),
                       "Created");
    }
}

void ApplicationLoopService::Run() {
    if (logger_) {
        logger_->Trace("ApplicationLoopService", "Run", "", "Entering");
        logger_->Info("ApplicationLoopService::Run: Starting main loop");
    }

    running_ = true;
    auto lastTime = std::chrono::high_resolution_clock::now();
    auto startTime = lastTime;

    while (running_) {
        auto currentTime = std::chrono::high_resolution_clock::now();

        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        float elapsedTime = std::chrono::duration<float>(currentTime - startTime).count();
        lastTime = currentTime;

        HandleEvents();
        ProcessFrame(deltaTime, elapsedTime);
    }

    if (logger_) {
        logger_->Info("ApplicationLoopService::Run: Exiting main loop");
        logger_->Trace("ApplicationLoopService", "Run", "", "Exiting");
    }
}

void ApplicationLoopService::HandleEvents() {
    if (logger_) {
        logger_->Trace("ApplicationLoopService", "HandleEvents");
    }
    if (inputService_) {
        if (logger_) {
            logger_->Trace("ApplicationLoopService", "HandleEvents", "resetInputState=true");
        }
        inputService_->ResetFrameState();
    }
    if (windowService_) {
        windowService_->PollEvents();
    }

    if (eventBus_) {
        eventBus_->ProcessQueue();
    }

    if (windowService_ && windowService_->ShouldClose()) {
        running_ = false;
    }
}

void ApplicationLoopService::ProcessFrame(float deltaTime, float elapsedTime) {
    if (logger_) {
        logger_->Trace("ApplicationLoopService", "ProcessFrame",
                       "deltaTime=" + std::to_string(deltaTime) +
                       ", elapsedTime=" + std::to_string(elapsedTime) +
                       ", renderCoordinatorAvailable=" + std::string(renderCoordinatorService_ ? "true" : "false"),
                       "Entering");
    }

    if (physicsService_) {
        physicsService_->StepSimulation(deltaTime);
    }

    if (sceneService_) {
        sceneService_->UpdateScene(deltaTime);
    }

    if (renderCoordinatorService_) {
        renderCoordinatorService_->RenderFrame(elapsedTime);
    }

    if (audioService_) {
        audioService_->Update();
    }

    if (inputService_) {
        inputService_->UpdateGuiInput();
    }

    if (logger_) {
        logger_->Trace("ApplicationLoopService", "ProcessFrame", "", "Exiting");
    }
}

}  // namespace sdl3cpp::services::impl
