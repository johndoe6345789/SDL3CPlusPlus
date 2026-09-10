#include "services/interfaces/workflow/rendering/workflow_render_prepare_step.hpp"
#include "services/interfaces/workflow/rendering/render_prepare_helpers.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

WorkflowRenderPrepareStep::WorkflowRenderPrepareStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowRenderPrepareStep::GetPluginId() const {
    return "render.prepare";
}

void WorkflowRenderPrepareStep::Execute(const WorkflowStepDefinition& step,
                                        WorkflowContext& context) {
    const glm::vec3 cameraPos = PrepareRenderCameraState(context);
    PrepareRenderShadowState(context);
    PrepareRenderLightingState(context);

    if (logger_) {
        logger_->Trace("WorkflowRenderPrepareStep", "Execute",
                       "cam=(" + std::to_string(cameraPos.x) + "," +
                       std::to_string(cameraPos.y) + "," +
                       std::to_string(cameraPos.z) + ")",
                       "Render state prepared");
    }
}

}  // namespace sdl3cpp::services::impl
