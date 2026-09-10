#pragma once

#include "services/interfaces/workflow/quake3/q3_nav_types.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// q3.bots.update's tunable parameters, each with the original defaults.
struct BotUpdateParams {
    float chaseRange        = 20.0f;
    float shootRange        = 6.0f;
    float moveSpeed         = 3.0f;
    int legIdle             = 162;  // Q3 keel model defaults
    int legRun              = 167;
    int legRunCount         = 8;
    int torsoStand          = 101;
    int torsoAttack         = 107;
    int torsoAttackCount    = 6;
    int shootIntervalFrames = 30;
    int replanFrames        = 60;  // A* replan interval
};

BotUpdateParams ReadBotUpdateParams(const WorkflowStepDefinition& step);

/// Prefers the explicit `q3.player_pos` override; falls back to
/// `camera.state.position`, or the origin if neither is set.
glm::vec3 ReadBotUpdatePlayerPosition(const WorkflowContext& context);

/// Null unless q3.nav.build has already populated the shared nav graph.
const q3::NavGraph* ReadBotUpdateNavGraph(const WorkflowContext& context);

/// True if nothing blocks the line from `from` to `to` in the physics world;
/// with no world, falls back to "close enough" (`distance < chaseRange`).
bool HasLineOfSightToPlayer(btDiscreteDynamicsWorld* world,
                            const glm::vec3& from, const glm::vec3& to,
                            float distance, float chaseRange);

/// idle / chase / shoot, from distance-to-player and line-of-sight.
std::string DetermineBotState(float distance, bool canSeePlayer,
                              float shootRange, float chaseRange);

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

/**
 * @brief Picks leg/torso animation frames and fires a shot when due.
 *
 * Appends a shot record (`bot_id`, `from`, `to`) to `shots` when the bot is
 * shooting and `shootIntervalFrames` have passed since its last shot.
 */
void SelectBotAnimationAndFire(nlohmann::json& bot, const std::string& state,
                               double elapsedSeconds, int globalFrame,
                               const glm::vec3& playerPos,
                               const BotUpdateParams& params,
                               nlohmann::json& shots);

/**
 * @brief Runs one frame of AI for one bot: state, movement, animation, fire.
 *
 * Composes the functions above in the order q3.bots.update always ran them.
 */
void UpdateOneBot(int botIndex, nlohmann::json& bot, const glm::vec3& playerPos,
                  const q3::NavGraph* navGraph, btDiscreteDynamicsWorld* world,
                  const BotUpdateParams& params, WorkflowContext& context,
                  double dt, double elapsedSeconds, int globalFrame,
                  nlohmann::json& shots);

}  // namespace sdl3cpp::services::impl
