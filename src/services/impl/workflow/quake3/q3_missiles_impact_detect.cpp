#include "services/interfaces/workflow/quake3/q3_missiles_impact_helpers.hpp"

namespace sdl3cpp::services::impl {

int Q3SplashDamage(const sdl3cpp::q3::Q3Missile& m,
                   const glm::vec3& targetPos) {
    const float dist = glm::length(targetPos - m.origin);
    if (dist >= m.splashRadius) return 0;
    return static_cast<int>(m.splashDamage * (1.f - dist / m.splashRadius));
}

glm::vec3 Q3BotPosition(const nlohmann::json& bot) {
    if (bot.contains("pos")) {
        const auto& p = bot["pos"];
        return glm::vec3(p[0].get<float>(), p[1].get<float>(),
                         p[2].get<float>());
    }
    return glm::vec3(0.f);
}

void DetectQ3MissileImpacts(
    btDiscreteDynamicsWorld* world,
    std::vector<sdl3cpp::q3::Q3Missile>& missiles,
    std::unordered_map<uint32_t, glm::vec3>& prevPositions) {
    if (!world) return;

    for (auto& m : missiles) {
        if (m.exploded) continue;

        auto prevIt = prevPositions.find(m.id);
        const glm::vec3 prev =
            (prevIt != prevPositions.end()) ? prevIt->second : m.origin;

        const btVector3 btFrom(prev.x, prev.y, prev.z);
        const btVector3 btTo(m.origin.x, m.origin.y, m.origin.z);

        // Only cast if the missile actually moved.
        if (btFrom.distance(btTo) > 0.0001f) {
            btCollisionWorld::ClosestRayResultCallback cb(btFrom, btTo);
            world->rayTest(btFrom, btTo, cb);
            if (cb.hasHit()) {
                // Snap origin to hit point for splash calculation.
                m.origin =
                    glm::vec3(cb.m_hitPointWorld.x(), cb.m_hitPointWorld.y(),
                              cb.m_hitPointWorld.z());
                m.exploded = true;
            }
        }

        // Update stored previous position for next frame.
        prevPositions[m.id] = m.origin;
    }
}

}  // namespace sdl3cpp::services::impl
