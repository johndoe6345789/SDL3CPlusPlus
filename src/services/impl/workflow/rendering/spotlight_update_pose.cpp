#include "services/interfaces/workflow/rendering/spotlight_update_pose.hpp"
#include "services/interfaces/workflow/rendering/viewmodel_transform.hpp"

#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

namespace {

SpotlightPose ComputeViewmodelSpotlightPose(const nlohmann::json& spot,
    const glm::vec3& offset, WorkflowContext& context,
    const std::shared_ptr<ILogger>& logger) {
    // Place the light at the model's lens by running the point through
    // the same matrix draw.viewmodel uses, so the beam cannot drift
    // from the torch it is supposed to come from.
    const auto viewMatrix =
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f));
    const auto cameraPos =
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.0f));
    const std::string mesh = spot.value("mesh", std::string());
    const auto* meta = context.TryGet<nlohmann::json>("plane_" + mesh);
    const float lensY = meta ? meta->value("lens_y", 0.0f) : 0.0f;

    const auto rot = spot.value("rotation", std::vector<float>{});
    const glm::vec3 rotation(rot.size() > 0 ? rot[0] : 0.0f,
        rot.size() > 1 ? rot[1] : 0.0f, rot.size() > 2 ? rot[2] : 0.0f);
    const float scale = spot.value("scale", 1.0f);

    const glm::mat4 model = rendering::BuildViewmodelMatrix(
        viewMatrix, cameraPos, offset, rotation, scale);

    const glm::vec3 lens =
        glm::vec3(model * glm::vec4(0.0f, lensY, 0.0f, 1.0f));
    const glm::vec3 dir = glm::normalize(
        glm::vec3(model * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));

    // The lens sits ahead of the eye, so against a wall it can land on
    // the far side and light the room beyond it. The player's collision
    // capsule cannot overlap geometry, so any point within its radius is
    // guaranteed to be in open space: keep the origin inside that.
    const float maxOffset = spot.value("max_origin_offset", 0.25f);
    const glm::vec3 fromEye = lens - cameraPos;
    const float reach = glm::length(fromEye);
    const glm::vec3 pos = (reach > maxOffset && reach > 0.0f)
        ? cameraPos + fromEye * (maxOffset / reach) : lens;

    if (logger && !meta) {
        logger->Warn("spotlight.update: mesh '" + mesh +
            "' has no lens_y; beam starts at the model origin");
    }
    return SpotlightPose{pos, dir};
}

SpotlightPose ComputeCameraSpotlightPose(
    const nlohmann::json& spot, const glm::vec3& offset,
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

SpotlightPose ComputeWorldSpotlightPose(
    const nlohmann::json& spot, const glm::vec3& offset) {
    const auto p = spot.value("position", std::vector<float>{});
    const auto d = spot.value("direction", std::vector<float>{});
    const glm::vec3 pos = glm::vec3(p.size() > 0 ? p[0] : 0.0f,
        p.size() > 1 ? p[1] : 0.0f, p.size() > 2 ? p[2] : 0.0f) + offset;
    const glm::vec3 dir = glm::normalize(glm::vec3(
        d.size() > 0 ? d[0] : 0.0f, d.size() > 1 ? d[1] : 0.0f,
        d.size() > 2 ? d[2] : -1.0f));
    return SpotlightPose{pos, dir};
}

}  // namespace

SpotlightPose ComputeSpotlightPose(const nlohmann::json& spot,
    const glm::vec3& offset, WorkflowContext& context,
    const std::shared_ptr<ILogger>& logger) {
    const std::string attach = spot.value("attach", std::string());
    if (attach == "viewmodel") {
        return ComputeViewmodelSpotlightPose(spot, offset, context, logger);
    }
    if (attach == "camera") {
        return ComputeCameraSpotlightPose(spot, offset, context);
    }
    return ComputeWorldSpotlightPose(spot, offset);
}

}  // namespace sdl3cpp::services::impl
