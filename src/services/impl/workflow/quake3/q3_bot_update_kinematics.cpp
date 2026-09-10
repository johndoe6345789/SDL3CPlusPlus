#include "services/interfaces/workflow/quake3/q3_bot_update_kinematics.hpp"
#include "services/interfaces/workflow/quake3/q3_bot_update_animation.hpp"
#include "services/interfaces/workflow/quake3/q3_bot_update_movement.hpp"
#include "services/interfaces/workflow/quake3/q3_bot_update_sensing.hpp"

#include <cmath>

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

    bot["yaw"] = std::atan2(toPlayer.x, toPlayer.z);

    const bool canSeePlayer =
        HasLineOfSightToPlayer(world, bpos, playerPos, dist, params.chaseRange);
    const std::string state = DetermineBotState(
        dist, canSeePlayer, params.shootRange, params.chaseRange);
    bot["state"] = state;

    if (state == "chase") {
        UpdateBotChaseMovement(navGraph, context, botIndex, bot, playerPos,
                               params, dt, globalFrame);
    }
    SelectBotAnimationAndFire(bot, state, elapsedSeconds, globalFrame,
                              playerPos, params, shots);
}

}  // namespace sdl3cpp::services::impl
