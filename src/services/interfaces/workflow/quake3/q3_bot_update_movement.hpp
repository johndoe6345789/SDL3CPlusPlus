#pragma once

#include "services/interfaces/workflow/quake3/q3_bot_update_params.hpp"
#include "services/interfaces/workflow/quake3/q3_nav_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/**
 * @brief Advances a chasing bot toward the player along its cached nav path.
 *
 * Re-plans via A* when the path is empty or `replanFrames` have passed since
 * the last plan; steers toward the path's next waypoint (or straight at the
 * player with no nav graph) and pops the waypoint once close to it. Mutates
 * `bot["pos"]` and `bot["last_plan_frame"]`, and the bot's cached path in the
 * context under `q3.bot_path_{botIndex}`.
 */
void UpdateBotChaseMovement(const q3::NavGraph* navGraph,
                            WorkflowContext& context, int botIndex,
                            nlohmann::json& bot, const glm::vec3& playerPos,
                            const BotUpdateParams& params, double dt,
                            int globalFrame);

}  // namespace sdl3cpp::services::impl
