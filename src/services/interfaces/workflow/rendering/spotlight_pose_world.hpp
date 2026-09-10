#pragma once

#include "services/interfaces/workflow/rendering/spotlight_pose_types.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// Raw world-space "position"/"direction" from `spot`, offset added to
/// position (the fallback for any `attach` value other than "viewmodel"
/// or "camera").
SpotlightPose ComputeWorldSpotlightPose(const nlohmann::json& spot,
                                        const glm::vec3& offset);

}  // namespace sdl3cpp::services::impl
