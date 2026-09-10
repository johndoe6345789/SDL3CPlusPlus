#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/rendering/spotlight_pose_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/**
 * @brief Places the spotlight at the viewmodel mesh's lens, pulled back to
 *        stay within the player's collision radius of the eye so it can't
 *        poke through a wall it is flush against.
 *
 * Runs the lens point through the same matrix draw.viewmodel uses, so the
 * beam cannot drift from the torch it is supposed to come from.
 */
SpotlightPose ComputeViewmodelSpotlightPose(
    const nlohmann::json& spot, const glm::vec3& offset,
    WorkflowContext& context, const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
