#include "services/interfaces/workflow/workflow_generic_steps/camera_fps_update_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {

CameraFpsUpdateParams ReadCameraFpsUpdateParams(
    const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver paramResolver;
    CameraFpsUpdateParams params;

    auto readParam = [&](const char* name, auto& out) {
        if (const auto* p = paramResolver.FindParameter(step, name)) {
            if (p->type == WorkflowParameterValue::Type::Number) {
                out = static_cast<std::remove_reference_t<decltype(out)>>(
                    p->numberValue);
            }
        }
    };
    readParam("sensitivity", params.sensitivity);
    readParam("eye_height", params.eyeHeight);
    readParam("fov", params.fovDeg);
    readParam("near", params.nearPlane);
    readParam("far", params.farPlane);
    return params;
}

}  // namespace sdl3cpp::services::impl
