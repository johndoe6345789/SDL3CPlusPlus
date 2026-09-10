#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_set_pose_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/camera_set_pose_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {

WorkflowCameraSetPoseStep::WorkflowCameraSetPoseStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowCameraSetPoseStep::GetPluginId() const {
    return "camera.set_pose";
}

void WorkflowCameraSetPoseStep::Execute(const WorkflowStepDefinition& step,
                                        WorkflowContext& context) {
    WorkflowStepIoResolver ioResolver;
    WorkflowStepParameterResolver parameterResolver;
    const std::string outputKey = ioResolver.GetRequiredOutputKey(step, "pose");

    CameraPose pose;
    pose.position = ReadCameraPoseVec3(step, context, parameterResolver,
                                       "position", pose.position);
    pose.lookAt   = ReadCameraPoseVec3(step, context, parameterResolver,
                                       "look_at", pose.lookAt);
    pose.up =
        ReadCameraPoseVec3(step, context, parameterResolver, "up", pose.up);
    pose.fovDegrees = ReadCameraPoseNumber(step, context, parameterResolver,
                                           "fov_degrees", pose.fovDegrees);
    pose.nearPlane  = ReadCameraPoseNumber(step, context, parameterResolver,
                                           "near", pose.nearPlane);
    pose.farPlane   = ReadCameraPoseNumber(step, context, parameterResolver,
                                           "far", pose.farPlane);

    context.Set(outputKey, pose);

    if (logger_) {
        logger_->Trace("WorkflowCameraSetPoseStep", "Execute",
                       "output=" + outputKey, "Set camera pose");
    }
}

}  // namespace sdl3cpp::services::impl
