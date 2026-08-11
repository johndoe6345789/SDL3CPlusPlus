#include "services/interfaces/workflow/quake3/workflow_q3_missiles_impact_step.hpp"
#include "services/interfaces/workflow/quake3/q3_missile_types.hpp"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <string>

namespace sdl3cpp::services::impl {

namespace {

// Splash damage: linear falloff to zero at splashRadius.
int SplashDamage(const sdl3cpp::q3::Q3Missile& m, const glm::vec3& targetPos) {
    const float dist = glm::length(targetPos - m.origin);
    if (dist >= m.splashRadius) return 0;
    return static_cast<int>(m.splashDamage * (1.f - dist / m.splashRadius));
}

glm::vec3 BotPosition(const nlohmann::json& bot) {
    if (bot.contains("pos")) {
        const auto& p = bot["pos"];
        return glm::vec3(p[0].get<float>(), p[1].get<float>(), p[2].get<float>());
    }
    return glm::vec3(0.f);
}

}  // namespace

WorkflowQ3MissilesImpactStep::WorkflowQ3MissilesImpactStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3MissilesImpactStep::GetPluginId() const {
    return "q3.missiles.impact";
}

void WorkflowQ3MissilesImpactStep::Execute(const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    auto missiles = context.Get<sdl3cpp::q3::MissileList>("q3.missiles", nullptr);
    if (!missiles || missiles->empty()) {
        // Clean up stale prev-position entries for finished missiles.
        prevPositions_.clear();
        return;
    }

    auto* world = context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    const glm::vec3 playerPos = context.Get<glm::vec3>("q3.player_pos", glm::vec3(0.f));
    auto bots     = context.Get<nlohmann::json>("q3.bots", nlohmann::json::array());
    auto botDamage = context.Get<nlohmann::json>("q3.bot_damage", nlohmann::json::object());
    int pendingDamage = context.Get<int>("q3.pending_damage", 0);

    // =====================================================================
    // Phase 1: detect impacts via raycast from prev→current position.
    // =====================================================================
    if (world) {
        for (auto& m : *missiles) {
            if (m.exploded) continue;

            auto prevIt = prevPositions_.find(m.id);
            const glm::vec3 prev = (prevIt != prevPositions_.end()) ? prevIt->second : m.origin;

            const btVector3 btFrom(prev.x, prev.y, prev.z);
            const btVector3 btTo(m.origin.x, m.origin.y, m.origin.z);

            // Only cast if the missile actually moved.
            if (btFrom.distance(btTo) > 0.0001f) {
                btCollisionWorld::ClosestRayResultCallback cb(btFrom, btTo);
                world->rayTest(btFrom, btTo, cb);
                if (cb.hasHit()) {
                    // Snap origin to hit point for splash calculation.
                    m.origin   = glm::vec3(cb.m_hitPointWorld.x(),
                                           cb.m_hitPointWorld.y(),
                                           cb.m_hitPointWorld.z());
                    m.exploded = true;
                }
            }

            // Update stored previous position for next frame.
            prevPositions_[m.id] = m.origin;
        }
    }

    // =====================================================================
    // Phase 2: splash damage for every exploded missile.
    // =====================================================================
    for (const auto& m : *missiles) {
        if (!m.exploded) continue;

        // Player splash.
        const int playerSplash = SplashDamage(m, playerPos);
        pendingDamage += playerSplash;

        // Bot splash.
        for (auto& [key, bot] : bots.items()) {
            const glm::vec3 botPos = BotPosition(bot);
            const int botSplash = SplashDamage(m, botPos);
            if (botSplash > 0) {
                const std::string botKey = key;
                botDamage[botKey] = botDamage.value(botKey, 0) + botSplash;
            }
        }

        // Remove this missile's prev-position entry — it's done.
        prevPositions_.erase(m.id);
    }

    // =====================================================================
    // Phase 3: remove all exploded missiles.
    // =====================================================================
    missiles->erase(
        std::remove_if(missiles->begin(), missiles->end(),
                       [](const sdl3cpp::q3::Q3Missile& m) { return m.exploded; }),
        missiles->end());

    context.Set("q3.missiles", missiles);
    context.Set<int>("q3.pending_damage", pendingDamage);
    context.Set("q3.bot_damage", botDamage);
}

}  // namespace sdl3cpp::services::impl
