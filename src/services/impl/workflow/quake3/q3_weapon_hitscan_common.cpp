#include "services/interfaces/workflow/quake3/q3_weapon_hitscan_internal.hpp"
#include "services/interfaces/workflow/quake3/q3_weapon_stats.hpp"

#include <cstdlib>

namespace sdl3cpp::q3::hitscan_detail {

glm::vec3 Tangent(const glm::vec3& dir) {
    const glm::vec3 up =
        (std::abs(dir.y) < 0.9f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    return glm::normalize(glm::cross(dir, up));
}

float JitterComponent(int seed) {
    return static_cast<float>((seed % 200) - 100) / 100.0f;
}

bool Raycast(btDiscreteDynamicsWorld* world, const btVector3& from,
            const btVector3& to, btVector3& hitPoint) {
    btCollisionWorld::ClosestRayResultCallback cb(from, to);
    world->rayTest(from, to, cb);
    if (cb.hasHit()) {
        hitPoint = cb.m_hitPointWorld;
        return true;
    }
    return false;
}

HitscanFireResult FireSingleRay(btDiscreteDynamicsWorld* world,
                                const std::string& weapon,
                                const glm::vec3& origin,
                                const glm::vec3& forwardN, float range) {
    const btVector3 from(origin.x, origin.y, origin.z);
    const btVector3 to =
        from + btVector3(forwardN.x, forwardN.y, forwardN.z) * range;
    btVector3 hit;
    if (!Raycast(world, from, to, hit)) {
        return {};
    }
    return {WeaponInstantHitDamage(weapon), true};
}

}  // namespace sdl3cpp::q3::hitscan_detail
