#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_look_at_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowCameraLookAtStep::WorkflowCameraLookAtStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowCameraLookAtStep::GetPluginId() const {
    return "camera.look_at";
}

void WorkflowCameraLookAtStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string poseKey = resolver.GetRequiredInputKey(step, "pose");
    const std::string targetKey = resolver.GetRequiredInputKey(step, "target");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "pose");

    const auto* pose = context.TryGet<CameraPose>(poseKey);
    const auto* target = context.TryGet<std::vector<double>>(targetKey);
    if (!pose || !target || target->size() != 3u) {
        throw std::runtime_error("camera.look_at requires pose and target[3] inputs");
    }

    CameraPose updated = *pose;
    updated.lookAt = {static_cast<float>((*target)[0]),
                      static_cast<float>((*target)[1]),
                      static_cast<float>((*target)[2])};
    context.Set(outputKey, updated);

    if (logger_) {
        logger_->Trace("WorkflowCameraLookAtStep", "Execute",
                       "input=" + poseKey +
                           ", output=" + outputKey,
                       "Updated camera look-at");
    }
}

}  // namespace sdl3cpp::services::impl
