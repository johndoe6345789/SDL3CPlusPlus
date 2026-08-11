#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_fps_update_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <vector>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowCameraFpsUpdateStep::WorkflowCameraFpsUpdateStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowCameraFpsUpdateStep::GetPluginId() const {
    return "camera.fps.update";
}

void WorkflowCameraFpsUpdateStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    // Read mouse sensitivity from parameters
    WorkflowStepParameterResolver paramResolver;
    float sensitivity = 0.003f;
    float eyeHeight = 1.5f;
    float fovDeg = 75.0f;
    float nearPlane = 0.1f;
    float farPlane = 500.0f;

    auto readParam = [&](const char* name, auto& out) {
        if (const auto* p = paramResolver.FindParameter(step, name)) {
            if (p->type == WorkflowParameterValue::Type::Number) {
                out = static_cast<std::remove_reference_t<decltype(out)>>(p->numberValue);
            }
        }
    };
    readParam("sensitivity", sensitivity);
    readParam("eye_height", eyeHeight);
    readParam("fov", fovDeg);
    readParam("near", nearPlane);
    readParam("far", farPlane);

    // Get accumulated mouse delta from input.poll
    float mouseRelX = context.Get<float>("input_mouse_rel_x", 0.0f);
    float mouseRelY = context.Get<float>("input_mouse_rel_y", 0.0f);

    // Update yaw/pitch (persisted across frames in context)
    float yaw = context.Get<float>("camera_yaw", 0.0f);
    float pitch = context.Get<float>("camera_pitch", 0.0f);

    yaw -= mouseRelX * sensitivity;
    pitch -= mouseRelY * sensitivity;  // Inverted Y

    // Clamp pitch to prevent flipping
    constexpr float maxPitch = 1.5f; // ~86 degrees
    pitch = std::clamp(pitch, -maxPitch, maxPitch);

    context.Set<float>("camera_yaw", yaw);
    context.Set<float>("camera_pitch", pitch);

    // Get player body position for eye position
    // Crouch/stand height override from physics.fps.move (if set)
    float actualEyeHeight = context.Get<float>("camera_eye_height", eyeHeight);

    auto playerName = context.GetString("physics_player_body", "");
    glm::vec3 eyePos(0.0f, actualEyeHeight, 0.0f);

    if (!playerName.empty()) {
        auto* body = context.Get<btRigidBody*>("physics_body_" + playerName, nullptr);
        if (body) {
            btTransform transform;
            body->getMotionState()->getWorldTransform(transform);
            btVector3 pos = transform.getOrigin();
            eyePos = glm::vec3(pos.x(), pos.y() + actualEyeHeight, pos.z());
        }
    }

    // Build look direction from yaw/pitch
    glm::vec3 front;
    front.x = std::cos(pitch) * (-std::sin(yaw));
    front.y = std::sin(pitch);
    front.z = std::cos(pitch) * (-std::cos(yaw));
    front = glm::normalize(front);

    // Build view matrix
    glm::mat4 view = glm::lookAt(eyePos, eyePos + front, glm::vec3(0.0f, 1.0f, 0.0f));

    // Build projection matrix
    auto fw = context.Get<uint32_t>("frame_width", 1024u);
    auto fh = context.Get<uint32_t>("frame_height", 768u);
    float aspect = static_cast<float>(fw) / static_cast<float>(fh > 0 ? fh : 1);
    glm::mat4 proj = glm::perspective(glm::radians(fovDeg), aspect, nearPlane, farPlane);

    // Store as camera.state JSON (same format render.cube_grid expects)
    std::vector<float> viewVec(16), projVec(16);
    std::memcpy(viewVec.data(), glm::value_ptr(view), 16 * sizeof(float));
    std::memcpy(projVec.data(), glm::value_ptr(proj), 16 * sizeof(float));

    nlohmann::json cameraState = {
        {"view", viewVec},
        {"projection", projVec},
        {"position", {eyePos.x, eyePos.y, eyePos.z}},
        {"front", {front.x, front.y, front.z}}
    };
    context.Set("camera.state", cameraState);
}

}  // namespace sdl3cpp::services::impl
