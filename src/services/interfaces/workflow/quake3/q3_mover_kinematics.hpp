#pragma once

#include "services/interfaces/workflow/quake3/q3_mover_types.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/**
 * @brief Advances one mover's open/close state machine by `dt` seconds.
 *
 * AtPos1 triggers MovingTo2 when the player is within 2.5 units of pos1;
 * AtPos2 waits `waitTime` seconds before returning via MovingTo1. Updates
 * `m.currentPos`/`m.velocity` in place.
 *
 * @return The mover's velocity, to add to the player's push if the
 *         player is within 1.5 units of `m.currentPos` while moving —
 *         zero otherwise (including while stationary).
 */
glm::vec3 UpdateQ3Mover(sdl3cpp::q3::Q3Mover& m, const glm::vec3& playerPos,
                        float dt);

}  // namespace sdl3cpp::services::impl
