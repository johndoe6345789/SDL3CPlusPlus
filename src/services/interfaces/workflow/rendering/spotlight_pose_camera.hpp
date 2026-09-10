#pragma once

#include "services/interfaces/workflow/rendering/spotlight_pose_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// Offsets the spotlight from the camera in camera axes, aimed either at
/// `spot["aim_distance"]` ahead or straight down the camera's forward.
SpotlightPose ComputeCameraSpotlightPose(const nlohmann::json& spot,
                                         const glm::vec3& offset,
                                         WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
