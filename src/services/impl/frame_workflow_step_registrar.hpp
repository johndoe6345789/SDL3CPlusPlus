#pragma once

#include "../interfaces/i_logger.hpp"
#include "../interfaces/i_workflow_step_registry.hpp"
#include "../interfaces/workflow_definition.hpp"

#include <memory>

namespace sdl3cpp::services {
class IAudioService;
class IInputService;
class IPhysicsService;
class IRenderCoordinatorService;
class ISceneService;
}

namespace sdl3cpp::services::impl {

class FrameWorkflowStepRegistrar {
public:
    FrameWorkflowStepRegistrar(std::shared_ptr<ILogger> logger,
                               std::shared_ptr<IAudioService> audioService,
                               std::shared_ptr<IInputService> inputService,
                               std::shared_ptr<IPhysicsService> physicsService,
                               std::shared_ptr<ISceneService> sceneService,
                               std::shared_ptr<IRenderCoordinatorService> renderService);

    void RegisterUsedSteps(const WorkflowDefinition& workflow,
                           const std::shared_ptr<IWorkflowStepRegistry>& registry) const;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IAudioService> audioService_;
    std::shared_ptr<IInputService> inputService_;
    std::shared_ptr<IPhysicsService> physicsService_;
    std::shared_ptr<ISceneService> sceneService_;
    std::shared_ptr<IRenderCoordinatorService> renderService_;
};

}  // namespace sdl3cpp::services::impl
