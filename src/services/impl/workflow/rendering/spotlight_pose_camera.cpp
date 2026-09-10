#include "services/interfaces/workflow/rendering/spotlight_pose_camera.hpp"
#include "services/interfaces/workflow/rendering/viewmodel_transform.hpp"

namespace sdl3cpp::services::impl {

SpotlightPose ComputeCameraSpotlightPose(const nlohmann::json& spot,
                                         const glm::vec3& offset,
                                         WorkflowContext& context) {
    const auto viewMatrix =
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f));
    const auto cameraPos =
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.0f));
    const rendering::CameraBasis basis =
        rendering::ExtractCameraBasis(viewMatrix);

    const glm::vec3 pos = cameraPos + basis.right * offset.x +
                          basis.up * offset.y + basis.forward * (-offset.z);

    const float aimDist = spot.value("aim_distance", 0.0f);
    if (aimDist <= 0.0f) return SpotlightPose{pos, basis.forward};

    const glm::vec3 aimTarget = cameraPos + basis.forward * aimDist;
    return SpotlightPose{pos, glm::normalize(aimTarget - pos)};
}

}  // namespace sdl3cpp::services::impl
