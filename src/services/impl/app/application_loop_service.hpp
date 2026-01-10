#pragma once

#include "services/interfaces/i_application_loop_service.hpp"
#include "services/interfaces/i_frame_workflow_service.hpp"
#include "services/interfaces/i_audio_service.hpp"
#include "services/interfaces/i_crash_recovery_service.hpp"
#include "services/interfaces/i_input_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_physics_service.hpp"
#include "services/interfaces/i_render_coordinator_service.hpp"
#include "services/interfaces/i_scene_service.hpp"
#include "services/interfaces/i_window_service.hpp"
#include "../../../events/i_event_bus.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

class ApplicationLoopService : public IApplicationLoopService {
public:
    ApplicationLoopService(std::shared_ptr<ILogger> logger,
                           std::shared_ptr<IWindowService> windowService,
                           std::shared_ptr<events::IEventBus> eventBus,
                           std::shared_ptr<IInputService> inputService,
                           std::shared_ptr<IPhysicsService> physicsService,
                           std::shared_ptr<ISceneService> sceneService,
                           std::shared_ptr<IRenderCoordinatorService> renderCoordinatorService,
                          std::shared_ptr<IAudioService> audioService,
                          std::shared_ptr<IFrameWorkflowService> frameWorkflowService,
                          std::shared_ptr<ICrashRecoveryService> crashRecoveryService);
    ~ApplicationLoopService() override = default;

    void Run() override;

private:
    void HandleEvents();
    void ProcessFrame(float deltaTime, float elapsedTime);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IWindowService> windowService_;
    std::shared_ptr<events::IEventBus> eventBus_;
    std::shared_ptr<IInputService> inputService_;
    std::shared_ptr<IPhysicsService> physicsService_;
    std::shared_ptr<ISceneService> sceneService_;
    std::shared_ptr<IRenderCoordinatorService> renderCoordinatorService_;
    std::shared_ptr<IAudioService> audioService_;
    std::shared_ptr<IFrameWorkflowService> frameWorkflowService_;
    std::shared_ptr<ICrashRecoveryService> crashRecoveryService_;
    bool running_ = false;
    double lastMemoryCheckSeconds_ = 0.0;
    double memoryCheckIntervalSeconds_ = 1.0;
};

}  // namespace sdl3cpp::services::impl
