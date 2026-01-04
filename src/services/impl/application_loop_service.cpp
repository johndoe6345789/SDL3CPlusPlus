#include "application_loop_service.hpp"

#include <chrono>

namespace sdl3cpp::services::impl {

ApplicationLoopService::ApplicationLoopService(std::shared_ptr<ILogger> logger,
                                               std::shared_ptr<IWindowService> windowService,
                                               std::shared_ptr<events::IEventBus> eventBus,
                                               std::shared_ptr<IInputService> inputService,
                                               std::shared_ptr<IPhysicsService> physicsService,
                                               std::shared_ptr<ISceneService> sceneService,
                                               std::shared_ptr<IAudioService> audioService)
    : logger_(std::move(logger)),
      windowService_(std::move(windowService)),
      eventBus_(std::move(eventBus)),
      inputService_(std::move(inputService)),
      physicsService_(std::move(physicsService)),
      sceneService_(std::move(sceneService)),
      audioService_(std::move(audioService)) {
    if (logger_) {
        logger_->Trace("ApplicationLoopService", "ApplicationLoopService", "", "Created");
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
    const auto timeout = std::chrono::seconds(5);

    while (running_) {
        auto currentTime = std::chrono::high_resolution_clock::now();

        if (currentTime - startTime > timeout) {
            if (logger_) {
                logger_->Info("ApplicationLoopService::Run: Timeout reached, exiting");
            }
            running_ = false;
            break;
        }

        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        HandleEvents();
        ProcessFrame(deltaTime);
    }

    if (logger_) {
        logger_->Info("ApplicationLoopService::Run: Exiting main loop");
        logger_->Trace("ApplicationLoopService", "Run", "", "Exiting");
    }
}

void ApplicationLoopService::HandleEvents() {
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

void ApplicationLoopService::ProcessFrame(float deltaTime) {
    if (logger_) {
        logger_->Trace("ApplicationLoopService", "ProcessFrame", "deltaTime=" + std::to_string(deltaTime), "Entering");
    }

    if (inputService_) {
        inputService_->ResetFrameState();
    }

    if (physicsService_) {
        physicsService_->StepSimulation(deltaTime);
    }

    if (sceneService_) {
        sceneService_->UpdateScene(deltaTime);
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
