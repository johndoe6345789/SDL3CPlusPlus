#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step_registry.hpp"
#include "services/interfaces/workflow_definition.hpp"

#include <memory>

namespace sdl3cpp::services {
class IAudioService;
class IConfigService;
class IInputService;
class IMeshService;
class IPhysicsService;
class IRenderCoordinatorService;
class ISceneService;
class ISoundboardStateService;
class IValidationTourService;
}

namespace sdl3cpp::services::impl {

class FrameWorkflowStepRegistrar {
public:
    FrameWorkflowStepRegistrar(std::shared_ptr<ILogger> logger,
                               std::shared_ptr<IConfigService> configService,
                               std::shared_ptr<IAudioService> audioService,
                               std::shared_ptr<IInputService> inputService,
                               std::shared_ptr<IMeshService> meshService,
                               std::shared_ptr<IPhysicsService> physicsService,
                               std::shared_ptr<ISceneService> sceneService,
                               std::shared_ptr<IRenderCoordinatorService> renderService,
                               std::shared_ptr<IValidationTourService> validationTourService,
                               std::shared_ptr<ISoundboardStateService> soundboardStateService);

    void RegisterUsedSteps(const WorkflowDefinition& workflow,
                           const std::shared_ptr<IWorkflowStepRegistry>& registry) const;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IConfigService> configService_;
    std::shared_ptr<IAudioService> audioService_;
    std::shared_ptr<IInputService> inputService_;
    std::shared_ptr<IMeshService> meshService_;
    std::shared_ptr<IPhysicsService> physicsService_;
    std::shared_ptr<ISceneService> sceneService_;
    std::shared_ptr<IRenderCoordinatorService> renderService_;
    std::shared_ptr<IValidationTourService> validationTourService_;
    std::shared_ptr<ISoundboardStateService> soundboardStateService_;
};

}  // namespace sdl3cpp::services::impl
