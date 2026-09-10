#pragma once

#include "services/interfaces/workflow/quake3/q3_bot_update_params.hpp"

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <string>

namespace sdl3cpp::services::impl {

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

}  // namespace sdl3cpp::services::impl
