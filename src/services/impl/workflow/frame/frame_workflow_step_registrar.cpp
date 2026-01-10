#include "frame_workflow_step_registrar.hpp"

#include "workflow_frame_audio_step.hpp"
#include "workflow_frame_begin_step.hpp"
#include "workflow_frame_camera_step.hpp"
#include "workflow_frame_bullet_physics_step.hpp"
#include "workflow_frame_gui_step.hpp"
#include "workflow_frame_physics_step.hpp"
#include "workflow_frame_render_step.hpp"
#include "workflow_frame_scene_step.hpp"
#include "workflow_generic_steps/workflow_list_filter_equals_step.hpp"
#include "workflow_generic_steps/workflow_list_map_add_step.hpp"
#include "workflow_generic_steps/workflow_list_reduce_sum_step.hpp"
#include "workflow_generic_steps/workflow_number_add_step.hpp"
#include "workflow_soundboard_audio_step.hpp"
#include "workflow_soundboard_catalog_scan_step.hpp"
#include "workflow_soundboard_gui_step.hpp"
#include "workflow_step_registry.hpp"
#include "workflow_generic_steps/workflow_value_copy_step.hpp"
#include "workflow_generic_steps/workflow_value_default_step.hpp"
#include "workflow_validation_checkpoint_step.hpp"

#include <stdexcept>
#include <unordered_set>

namespace sdl3cpp::services::impl {

FrameWorkflowStepRegistrar::FrameWorkflowStepRegistrar(std::shared_ptr<ILogger> logger,
                                                       std::shared_ptr<IConfigService> configService,
                                                       std::shared_ptr<IAudioService> audioService,
                                                       std::shared_ptr<IInputService> inputService,
                                                       std::shared_ptr<IPhysicsService> physicsService,
                                                       std::shared_ptr<ISceneService> sceneService,
                                                       std::shared_ptr<IRenderCoordinatorService> renderService,
                                                       std::shared_ptr<IValidationTourService> validationTourService,
                                                       std::shared_ptr<ISoundboardStateService> soundboardStateService)
    : logger_(std::move(logger)),
      configService_(std::move(configService)),
      audioService_(std::move(audioService)),
      inputService_(std::move(inputService)),
      physicsService_(std::move(physicsService)),
      sceneService_(std::move(sceneService)),
      renderService_(std::move(renderService)),
      validationTourService_(std::move(validationTourService)),
      soundboardStateService_(std::move(soundboardStateService)) {}

void FrameWorkflowStepRegistrar::RegisterUsedSteps(
    const WorkflowDefinition& workflow,
    const std::shared_ptr<IWorkflowStepRegistry>& registry) const {
    if (!registry) {
        throw std::runtime_error("FrameWorkflowStepRegistrar: registry is null");
    }
    std::unordered_set<std::string> plugins;
    for (const auto& step : workflow.steps) {
        plugins.insert(step.plugin);
    }

    if (plugins.contains("frame.begin")) {
        registry->RegisterStep(std::make_shared<WorkflowFrameBeginStep>(logger_));
    }
    if (plugins.contains("frame.physics")) {
        registry->RegisterStep(std::make_shared<WorkflowFramePhysicsStep>(physicsService_, logger_));
    }
    if (plugins.contains("frame.bullet_physics")) {
        registry->RegisterStep(std::make_shared<WorkflowFrameBulletPhysicsStep>(physicsService_, logger_));
    }
    if (plugins.contains("frame.camera")) {
        registry->RegisterStep(std::make_shared<WorkflowFrameCameraStep>(configService_, logger_));
    }
    if (plugins.contains("frame.scene")) {
        registry->RegisterStep(std::make_shared<WorkflowFrameSceneStep>(sceneService_, logger_));
    }
    if (plugins.contains("frame.render")) {
        registry->RegisterStep(std::make_shared<WorkflowFrameRenderStep>(renderService_, logger_));
    }
    if (plugins.contains("frame.audio")) {
        registry->RegisterStep(std::make_shared<WorkflowFrameAudioStep>(audioService_, logger_));
    }
    if (plugins.contains("frame.gui")) {
        registry->RegisterStep(std::make_shared<WorkflowFrameGuiStep>(inputService_, logger_));
    }
    if (plugins.contains("validation.tour.checkpoint")) {
        registry->RegisterStep(std::make_shared<WorkflowValidationCheckpointStep>(
            validationTourService_, logger_));
    }
    if (plugins.contains("soundboard.catalog.scan")) {
        registry->RegisterStep(std::make_shared<WorkflowSoundboardCatalogScanStep>(configService_, logger_));
    }
    if (plugins.contains("soundboard.gui")) {
        registry->RegisterStep(std::make_shared<WorkflowSoundboardGuiStep>(inputService_,
                                                                          configService_,
                                                                          soundboardStateService_,
                                                                          logger_));
    }
    if (plugins.contains("soundboard.audio")) {
        registry->RegisterStep(std::make_shared<WorkflowSoundboardAudioStep>(audioService_,
                                                                             soundboardStateService_,
                                                                             logger_));
    }
    if (plugins.contains("value.copy")) {
        registry->RegisterStep(std::make_shared<WorkflowValueCopyStep>(logger_));
    }
    if (plugins.contains("value.default")) {
        registry->RegisterStep(std::make_shared<WorkflowValueDefaultStep>(logger_));
    }
    if (plugins.contains("number.add")) {
        registry->RegisterStep(std::make_shared<WorkflowNumberAddStep>(logger_));
    }
    if (plugins.contains("list.filter.equals")) {
        registry->RegisterStep(std::make_shared<WorkflowListFilterEqualsStep>(logger_));
    }
    if (plugins.contains("list.map.add")) {
        registry->RegisterStep(std::make_shared<WorkflowListMapAddStep>(logger_));
    }
    if (plugins.contains("list.reduce.sum")) {
        registry->RegisterStep(std::make_shared<WorkflowListReduceSumStep>(logger_));
    }
}

}  // namespace sdl3cpp::services::impl
