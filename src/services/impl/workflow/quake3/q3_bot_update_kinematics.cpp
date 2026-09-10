#include "services/interfaces/workflow/quake3/q3_bot_update_kinematics.hpp"
#include "services/interfaces/workflow/quake3/q3_axes.hpp"
#include "services/interfaces/workflow/quake3/q3_bot_mover.hpp"
#include "services/interfaces/workflow/quake3/q3_bot_update_animation.hpp"
#include "services/interfaces/workflow/quake3/q3_bot_update_movement.hpp"
#include "services/interfaces/workflow/quake3/q3_bot_update_sensing.hpp"
#include "services/interfaces/workflow/quake3/q3_bot_usercmd.hpp"

namespace sdl3cpp::services::impl {

void UpdateOneBot(int botIndex, nlohmann::json& bot, const glm::vec3& playerPos,
                  const q3::NavGraph* navGraph, btDiscreteDynamicsWorld* world,
                  const BotUpdateParams& params, WorkflowContext& context,
                  double dt, double elapsedSeconds, int globalFrame,
                  nlohmann::json& shots) {
    const auto& posJ = bot["pos"];
    const glm::vec3 bpos(posJ[0].get<float>(), posJ[1].get<float>(),
                         posJ[2].get<float>());
    const glm::vec3 toPlayer = playerPos - bpos;
    const float dist         = glm::distance(playerPos, bpos);

    const float yaw = q3::YawTowards(toPlayer);
    bot["yaw"]      = yaw;

    const bool canSeePlayer =
        HasLineOfSightToPlayer(world, bpos, playerPos, dist, params.chaseRange);
    const std::string state = DetermineBotState(
        dist, canSeePlayer, params.shootRange, params.chaseRange);
    bot["state"] = state;

    glm::vec3 wishDir(0.0f);
    if (state == "chase") {
        wishDir = BotChaseDirection(navGraph, context, botIndex, bot, playerPos,
                                    params, globalFrame);
    }

    // Run pmove whether or not the bot wants to move: a bot standing
    // still still has to be held up by the floor, and one that has just
    // walked off a ledge still has to fall. Skipping this for idle bots
    // is what left them hovering wherever they spawned.
    const float speed = q3::BotSpeedFraction(params.moveSpeed);
    MoveBotThroughPmove(bot, q3::BotDirectionToUserCmd(wishDir, speed, yaw),
                        world, static_cast<float>(dt));

    SelectBotAnimationAndFire(bot, state, elapsedSeconds, globalFrame,
                              playerPos, params, shots);
}

}  // namespace sdl3cpp::services::impl
