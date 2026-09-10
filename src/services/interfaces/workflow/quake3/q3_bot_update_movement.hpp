#pragma once

#include "services/interfaces/workflow/quake3/q3_bot_update_params.hpp"
#include "services/interfaces/workflow/quake3/q3_nav_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/**
 * @brief Which way a chasing bot wants to go, as a unit world direction.
 *
 * Re-plans via A* when the path is empty or `replanFrames` have passed since
 * the last plan; steers toward the path's next waypoint (or straight at the
 * player with no nav graph) and pops the waypoint once close to it. Mutates
 * `bot["last_plan_frame"]` and the bot's cached path in the context under
 * `q3.bot_path_{botIndex}`.
 *
 * It deliberately does not move the bot. Wanting to be somewhere and
 * getting there are separate in Quake: this is the goal half, and pmove
 * decides what actually happens, so a bot cannot walk through a wall
 * just because its path said to. Zero when it has nowhere to go.
 */
glm::vec3 BotChaseDirection(const q3::NavGraph* navGraph,
                            WorkflowContext& context, int botIndex,
                            nlohmann::json& bot, const glm::vec3& playerPos,
                            const BotUpdateParams& params, int globalFrame);

}  // namespace sdl3cpp::services::impl
