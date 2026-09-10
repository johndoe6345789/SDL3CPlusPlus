#include "services/interfaces/workflow/quake3/q3_missiles_impact_helpers.hpp"

#include <string>

namespace sdl3cpp::services::impl {

void ApplyQ3MissileSplashDamage(
    const std::vector<sdl3cpp::q3::Q3Missile>& missiles,
    const glm::vec3& playerPos, const nlohmann::json& bots,
    nlohmann::json& botDamage, int& pendingDamage,
    std::unordered_map<uint32_t, glm::vec3>& prevPositions) {
    for (const auto& m : missiles) {
        if (!m.exploded) continue;

        // Player splash.
        pendingDamage += Q3SplashDamage(m, playerPos);

        // Bot splash.
        for (auto& [key, bot] : bots.items()) {
            const glm::vec3 botPos = Q3BotPosition(bot);
            const int botSplash    = Q3SplashDamage(m, botPos);
            if (botSplash > 0) {
                const std::string botKey = key;
                botDamage[botKey] = botDamage.value(botKey, 0) + botSplash;
            }
        }

        // Remove this missile's prev-position entry — it's done.
        prevPositions.erase(m.id);
    }
}

}  // namespace sdl3cpp::services::impl
