#pragma once

#include "services/interfaces/workflow/quake3/q3_bot_update_params.hpp"
#include "services/interfaces/workflow/quake3/q3_nav_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/**
 * @brief Runs one frame of AI for one bot: state, movement, animation, fire.
 *
 * Composes q3_bot_update_sensing / _movement / _animation in the order
 * q3.bots.update always ran them.
 */
void UpdateOneBot(int botIndex, nlohmann::json& bot, const glm::vec3& playerPos,
                  const q3::NavGraph* navGraph, btDiscreteDynamicsWorld* world,
                  const BotUpdateParams& params, WorkflowContext& context,
                  double dt, double elapsedSeconds, int globalFrame,
                  nlohmann::json& shots);

}  // namespace sdl3cpp::services::impl
