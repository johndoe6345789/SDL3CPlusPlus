#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// World-space position and (normalized) direction for a spotlight.
struct SpotlightPose {
    glm::vec3 position;
    glm::vec3 direction;
};

/**
 * @brief Computes where a spotlight sits and points this frame, per
 * `spot["attach"]`:
 * - "viewmodel": at the mesh's lens (via the same matrix draw.viewmodel
 *   uses), pulled back to stay within the player's collision radius of
 *   the eye so it can't poke through a wall it is flush against.
 * - "camera": offset from the camera in camera axes, aimed either at
 *   `aim_distance` ahead or straight down the camera's forward.
 * - anything else: raw world-space "position"/"direction" plus offset.
 */
SpotlightPose ComputeSpotlightPose(const nlohmann::json& spot,
    const glm::vec3& offset, WorkflowContext& context,
    const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
