#include "services/interfaces/workflow/rendering/spotlight_pose_viewmodel.hpp"
#include "services/interfaces/workflow/rendering/viewmodel_transform.hpp"

#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

SpotlightPose ComputeViewmodelSpotlightPose(
    const nlohmann::json& spot, const glm::vec3& offset,
    WorkflowContext& context, const std::shared_ptr<ILogger>& logger) {
    const auto viewMatrix =
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f));
    const auto cameraPos =
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.0f));
    const std::string mesh = spot.value("mesh", std::string());
    const auto* meta       = context.TryGet<nlohmann::json>("plane_" + mesh);
    const float lensY      = meta ? meta->value("lens_y", 0.0f) : 0.0f;

    const auto rot = spot.value("rotation", std::vector<float>{});
    const glm::vec3 rotation(rot.size() > 0 ? rot[0] : 0.0f,
                             rot.size() > 1 ? rot[1] : 0.0f,
                             rot.size() > 2 ? rot[2] : 0.0f);
    const float scale = spot.value("scale", 1.0f);

    const glm::mat4 model = rendering::BuildViewmodelMatrix(
        viewMatrix, cameraPos, offset, rotation, scale);

    const glm::vec3 lens =
        glm::vec3(model * glm::vec4(0.0f, lensY, 0.0f, 1.0f));
    const glm::vec3 dir =
        glm::normalize(glm::vec3(model * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));

    // The lens sits ahead of the eye, so against a wall it can land on
    // the far side and light the room beyond it. The player's collision
    // capsule cannot overlap geometry, so any point within its radius is
    // guaranteed to be in open space: keep the origin inside that.
    const float maxOffset   = spot.value("max_origin_offset", 0.25f);
    const glm::vec3 fromEye = lens - cameraPos;
    const float reach       = glm::length(fromEye);
    const glm::vec3 pos     = (reach > maxOffset && reach > 0.0f)
                                  ? cameraPos + fromEye * (maxOffset / reach)
                                  : lens;

    if (logger && !meta) {
        logger->Warn("spotlight.update: mesh '" + mesh +
                     "' has no lens_y; beam starts at the model origin");
    }
    return SpotlightPose{pos, dir};
}

}  // namespace sdl3cpp::services::impl
