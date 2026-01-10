#pragma once

#include "services/interfaces/i_frame_workflow_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_audio_service.hpp"
#include "services/interfaces/i_config_service.hpp"
#include "services/interfaces/i_input_service.hpp"
#include "services/interfaces/i_mesh_service.hpp"
#include "services/interfaces/i_physics_service.hpp"
#include "services/interfaces/i_render_coordinator_service.hpp"
#include "services/interfaces/i_scene_service.hpp"
#include "services/interfaces/i_soundboard_state_service.hpp"
#include "services/interfaces/i_validation_tour_service.hpp"

#include "../workflow_executor.hpp"
#include "../workflow_definition_parser.hpp"
#include "../workflow_step_registry.hpp"

#include <filesystem>
#include <memory>

namespace sdl3cpp::services::impl {

class FrameWorkflowService final : public IFrameWorkflowService {
public:
    FrameWorkflowService(std::shared_ptr<ILogger> logger,
                         std::shared_ptr<IConfigService> configService,
                         std::shared_ptr<IAudioService> audioService,
                         std::shared_ptr<IInputService> inputService,
                         std::shared_ptr<IMeshService> meshService,
                         std::shared_ptr<IPhysicsService> physicsService,
                         std::shared_ptr<ISceneService> sceneService,
                         std::shared_ptr<IRenderCoordinatorService> renderService,
                         std::shared_ptr<IValidationTourService> validationTourService,
                         std::shared_ptr<ISoundboardStateService> soundboardStateService,
                         const std::filesystem::path& templatePath = {});

    void ExecuteFrame(float deltaTime, float elapsedTime) override;

private:
    WorkflowDefinition workflow_;
    std::shared_ptr<WorkflowStepRegistry> registry_;
    WorkflowExecutor executor_;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
