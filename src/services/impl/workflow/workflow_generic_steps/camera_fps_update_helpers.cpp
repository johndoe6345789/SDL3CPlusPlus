#include "services/interfaces/workflow/workflow_generic_steps/camera_fps_update_helpers.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstring>
#include <vector>

namespace sdl3cpp::services::impl {

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
