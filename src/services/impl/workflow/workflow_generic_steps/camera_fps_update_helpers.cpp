#include "services/interfaces/workflow/workflow_generic_steps/camera_fps_update_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <btBulletDynamicsCommon.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

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

YawPitch UpdateCameraFpsYawPitch(WorkflowContext& context, float sensitivity) {
    const float mouseRelX = context.Get<float>("input_mouse_rel_x", 0.0f);
    const float mouseRelY = context.Get<float>("input_mouse_rel_y", 0.0f);

    float yaw   = context.Get<float>("camera_yaw", 0.0f);
    float pitch = context.Get<float>("camera_pitch", 0.0f);

    yaw -= mouseRelX * sensitivity;
    pitch -= mouseRelY * sensitivity;  // Inverted Y.

    // Clamp pitch to prevent flipping.
    constexpr float maxPitch = 1.5f;  // ~86 degrees.
    pitch                    = std::clamp(pitch, -maxPitch, maxPitch);

    context.Set<float>("camera_yaw", yaw);
    context.Set<float>("camera_pitch", pitch);
    return YawPitch{yaw, pitch};
}

glm::vec3 ComputeCameraFpsEyePosition(const WorkflowContext& context,
                                      float eyeHeight) {
    const float actualEyeHeight =
        context.Get<float>("camera_eye_height", eyeHeight);

    auto playerName = context.GetString("physics_player_body", "");
    glm::vec3 eyePos(0.0f, actualEyeHeight, 0.0f);

    if (!playerName.empty()) {
        auto* body =
            context.Get<btRigidBody*>("physics_body_" + playerName, nullptr);
        if (body) {
            btTransform transform;
            body->getMotionState()->getWorldTransform(transform);
            btVector3 pos = transform.getOrigin();
            eyePos = glm::vec3(pos.x(), pos.y() + actualEyeHeight, pos.z());
        }
    }
    return eyePos;
}

nlohmann::json BuildCameraFpsState(const WorkflowContext& context,
                                   const glm::vec3& eyePos,
                                   const YawPitch& yawPitch,
                                   const CameraFpsUpdateParams& params) {
    // Build look direction from yaw/pitch.
    glm::vec3 front;
    front.x = std::cos(yawPitch.pitch) * (-std::sin(yawPitch.yaw));
    front.y = std::sin(yawPitch.pitch);
    front.z = std::cos(yawPitch.pitch) * (-std::cos(yawPitch.yaw));
    front   = glm::normalize(front);

    const glm::mat4 view =
        glm::lookAt(eyePos, eyePos + front, glm::vec3(0.0f, 1.0f, 0.0f));

    auto fw = context.Get<uint32_t>("frame_width", 1024u);
    auto fh = context.Get<uint32_t>("frame_height", 768u);
    const float aspect =
        static_cast<float>(fw) / static_cast<float>(fh > 0 ? fh : 1);
    const glm::mat4 proj = glm::perspective(glm::radians(params.fovDeg), aspect,
                                            params.nearPlane, params.farPlane);

    // Store as camera.state JSON (same format render.cube_grid expects).
    std::vector<float> viewVec(16), projVec(16);
    std::memcpy(viewVec.data(), glm::value_ptr(view), 16 * sizeof(float));
    std::memcpy(projVec.data(), glm::value_ptr(proj), 16 * sizeof(float));

    return nlohmann::json{{"view", viewVec},
                          {"projection", projVec},
                          {"position", {eyePos.x, eyePos.y, eyePos.z}},
                          {"front", {front.x, front.y, front.z}}};
}

}  // namespace sdl3cpp::services::impl
