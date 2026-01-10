#include "frame_workflow_step_registrar.hpp"

#include "workflow_frame_audio_step.hpp"
#include "workflow_frame_begin_step.hpp"
#include "workflow_frame_bullet_physics_step.hpp"
#include "workflow_frame_gui_step.hpp"
#include "workflow_frame_physics_step.hpp"
#include "workflow_frame_render_step.hpp"
#include "workflow_frame_scene_step.hpp"
#include "workflow_step_registry.hpp"

#include <stdexcept>
#include <unordered_set>

namespace sdl3cpp::services::impl {

FrameWorkflowStepRegistrar::FrameWorkflowStepRegistrar(std::shared_ptr<ILogger> logger,
                                                       std::shared_ptr<IAudioService> audioService,
                                                       std::shared_ptr<IInputService> inputService,
                                                       std::shared_ptr<IPhysicsService> physicsService,
                                                       std::shared_ptr<ISceneService> sceneService,
                                                       std::shared_ptr<IRenderCoordinatorService> renderService)
    : logger_(std::move(logger)),
      audioService_(std::move(audioService)),
      inputService_(std::move(inputService)),
      physicsService_(std::move(physicsService)),
      sceneService_(std::move(sceneService)),
      renderService_(std::move(renderService)) {}

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
}

}  // namespace sdl3cpp::services::impl
