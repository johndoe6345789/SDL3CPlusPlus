#include "services/interfaces/workflow/workflow_camera_view_state_builder.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace sdl3cpp::services::impl {
namespace {
std::array<float, 16> ToArray(const glm::mat4& matrix) {
    std::array<float, 16> result{};
    std::memcpy(result.data(), glm::value_ptr(matrix), sizeof(float) * result.size());
    return result;
}
}  // namespace

ViewState BuildViewState(const CameraPose& pose, float aspect, const std::shared_ptr<ILogger>& logger) {
    if (logger) {
        logger->Trace("WorkflowCameraViewStateBuilder", "BuildViewState", "Entry");
    }
    glm::vec3 position(pose.position[0], pose.position[1], pose.position[2]);
    glm::vec3 lookAt(pose.lookAt[0], pose.lookAt[1], pose.lookAt[2]);
    glm::vec3 up(pose.up[0], pose.up[1], pose.up[2]);
    glm::mat4 view = glm::lookAt(position, lookAt, up);
    float safeAspect = aspect <= 0.0f ? 1.0f : aspect;
    glm::mat4 proj = glm::perspective(glm::radians(pose.fovDegrees),
                                      safeAspect,
                                      pose.nearPlane,
                                      pose.farPlane);

    ViewState state{};
    state.view = ToArray(view);
    state.proj = ToArray(proj);
    state.viewProj = ToArray(proj * view);
    state.cameraPosition = pose.position;
    return state;
}

}  // namespace sdl3cpp::services::impl
