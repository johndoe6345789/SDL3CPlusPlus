#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_fps_update_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/camera_fps_update_helpers.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

WorkflowCameraFpsUpdateStep::WorkflowCameraFpsUpdateStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowCameraFpsUpdateStep::GetPluginId() const {
    return "camera.fps.update";
}

void WorkflowCameraFpsUpdateStep::Execute(const WorkflowStepDefinition& step,
                                          WorkflowContext& context) {
    const CameraFpsUpdateParams params = ReadCameraFpsUpdateParams(step);

    const YawPitch yawPitch =
        UpdateCameraFpsYawPitch(context, params.sensitivity);
    const glm::vec3 eyePos =
        ComputeCameraFpsEyePosition(context, params.eyeHeight);

    const nlohmann::json cameraState =
        BuildCameraFpsState(context, eyePos, yawPitch, params);
    context.Set("camera.state", cameraState);
}

}  // namespace sdl3cpp::services::impl
