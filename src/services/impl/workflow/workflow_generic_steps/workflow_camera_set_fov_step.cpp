#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_set_fov_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowCameraSetFovStep::WorkflowCameraSetFovStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowCameraSetFovStep::GetPluginId() const {
    return "camera.set_fov";
}

void WorkflowCameraSetFovStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string poseKey = resolver.GetRequiredInputKey(step, "pose");
    const std::string fovKey = resolver.GetRequiredInputKey(step, "fov_degrees");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "pose");

    const auto* pose = context.TryGet<CameraPose>(poseKey);
    const auto* fov = context.TryGet<double>(fovKey);
    if (!pose || !fov) {
        throw std::runtime_error("camera.set_fov requires pose and fov_degrees inputs");
    }

    CameraPose updated = *pose;
    updated.fovDegrees = static_cast<float>(*fov);
    context.Set(outputKey, updated);

    if (logger_) {
        logger_->Trace("WorkflowCameraSetFovStep", "Execute",
                       "input=" + poseKey +
                           ", output=" + outputKey,
                       "Updated camera FOV");
    }
}

}  // namespace sdl3cpp::services::impl
