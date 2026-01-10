#include "workflow_frame_scene_step.hpp"
#include "../workflow_step_io_resolver.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowFrameSceneStep::WorkflowFrameSceneStep(std::shared_ptr<ISceneService> sceneService,
                                               std::shared_ptr<ILogger> logger)
    : sceneService_(std::move(sceneService)),
      logger_(std::move(logger)) {}

std::string WorkflowFrameSceneStep::GetPluginId() const {
    return "frame.scene";
}

void WorkflowFrameSceneStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!sceneService_) {
        throw std::runtime_error("frame.scene requires an ISceneService");
    }
    WorkflowStepIoResolver resolver;
    const std::string deltaKey = resolver.GetRequiredInputKey(step, "delta");
    const auto* delta = context.TryGet<double>(deltaKey);
    if (!delta) {
        throw std::runtime_error("frame.scene missing delta input");
    }
    sceneService_->UpdateScene(static_cast<float>(*delta));
    if (logger_) {
        logger_->Trace("WorkflowFrameSceneStep", "Execute",
                       "delta=" + std::to_string(*delta),
                       "Scene updated");
    }
}

}  // namespace sdl3cpp::services::impl
