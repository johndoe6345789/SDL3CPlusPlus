#include "services/interfaces/workflow/rendering/spotlight_update_pose.hpp"
#include "services/interfaces/workflow/rendering/spotlight_pose_camera.hpp"
#include "services/interfaces/workflow/rendering/spotlight_pose_viewmodel.hpp"
#include "services/interfaces/workflow/rendering/spotlight_pose_world.hpp"

#include <string>

namespace sdl3cpp::services::impl {

SpotlightPose ComputeSpotlightPose(const nlohmann::json& spot,
                                   const glm::vec3& offset,
                                   WorkflowContext& context,
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
