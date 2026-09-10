#include "services/interfaces/workflow/quake3/q3_bot_update_animation.hpp"

namespace sdl3cpp::services::impl {

void SelectBotAnimationAndFire(nlohmann::json& bot, const std::string& state,
                               double elapsedSeconds, int globalFrame,
                               const glm::vec3& playerPos,
                               const BotUpdateParams& params,
                               nlohmann::json& shots) {
    constexpr double kAnimFps = 15.0;
    const int baseFrame       = static_cast<int>(elapsedSeconds * kAnimFps);

    if (state == "chase") {
        bot["leg_frame"] =
            params.legRun +
            (params.legRunCount > 0 ? (baseFrame % params.legRunCount) : 0);
        bot["torso_frame"] = params.torsoStand;
        return;
    }

    if (state == "shoot") {
        bot["leg_frame"] = params.legIdle;
        bot["torso_frame"] =
            params.torsoAttack + (params.torsoAttackCount > 0
                                      ? (baseFrame % params.torsoAttackCount)
                                      : 0);

        const int lastShot = bot.value("last_shot", 0);
        if (globalFrame >= lastShot + params.shootIntervalFrames) {
            bot["last_shot"] = globalFrame;
            shots.push_back(
                {{"bot_id", bot["id"]},
                 {"from", bot["pos"]},
                 {"to", nlohmann::json::array(
                            {playerPos.x, playerPos.y, playerPos.z})}});
        }
        return;
    }

    bot["leg_frame"]   = params.legIdle;
    bot["torso_frame"] = params.torsoStand;
}

}  // namespace sdl3cpp::services::impl
