#include "services/interfaces/workflow/quake3/q3_weapon_hitscan.hpp"
#include "services/interfaces/workflow/quake3/q3_weapon_stats.hpp"

#include <cstdlib>

namespace sdl3cpp::q3 {
namespace {

constexpr float kHitscanRange = 120.0f;

/// Perpendicular tangent to a direction vector, for spread patterns.
glm::vec3 Tangent(const glm::vec3& dir) {
    const glm::vec3 up =
        (std::abs(dir.y) < 0.9f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    return glm::normalize(glm::cross(dir, up));
}

/// Deterministic-ish jitter in [-1, 1], cheap enough to avoid <random>.
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

HitscanFireResult FireShotgun(btDiscreteDynamicsWorld* world,
                              const glm::vec3& origin,
                              const glm::vec3& forwardN) {
    // 11 pellets, random spread up to +/-0.04 world units in the tangent plane.
    const glm::vec3 tan1    = Tangent(forwardN);
    const glm::vec3 tan2    = glm::normalize(glm::cross(forwardN, tan1));
    constexpr float kSpread = 0.04f;
    constexpr int kPellets  = 11;

    HitscanFireResult result;
    for (int p = 0; p < kPellets; ++p) {
        const float jx = JitterComponent(p * 7 + 3) * kSpread;
        const float jy = JitterComponent(p * 13 + 7) * kSpread;
        const glm::vec3 pelletDir =
            glm::normalize(forwardN + tan1 * jx + tan2 * jy);
        const auto pelletResult = FireSingleRay(world, "weapon_shotgun", origin,
                                                pelletDir, kHitscanRange);
        result.damage += pelletResult.damage;
        result.hit = result.hit || pelletResult.hit;
    }
    return result;
}

HitscanFireResult FireRailgun(btDiscreteDynamicsWorld* world,
                              const glm::vec3& origin,
                              const glm::vec3& forwardN) {
    // Piercing: step past each hit up to 4 times.
    constexpr int kMaxPierces = 4;
    glm::vec3 rayOrigin       = origin;

    HitscanFireResult result;
    for (int pierce = 0; pierce < kMaxPierces; ++pierce) {
        const btVector3 from(rayOrigin.x, rayOrigin.y, rayOrigin.z);
        const btVector3 to =
            from +
            btVector3(forwardN.x, forwardN.y, forwardN.z) * kHitscanRange;
        btVector3 hit;
        if (!Raycast(world, from, to, hit)) {
            break;
        }
        result.damage += WeaponInstantHitDamage("weapon_railgun");
        result.hit = true;
        // Step slightly past the hit point so the next iteration continues.
        rayOrigin = glm::vec3(hit.x(), hit.y(), hit.z()) + forwardN * 0.05f;
    }
    return result;
}

}  // namespace

HitscanFireResult FireHitscanWeapon(btDiscreteDynamicsWorld* world,
                                    const std::string& weapon,
                                    const glm::vec3& origin,
                                    const glm::vec3& forward) {
    const glm::vec3 forwardN = glm::normalize(forward);

    if (weapon == "weapon_machinegun") {
        return FireSingleRay(world, weapon, origin, forwardN, kHitscanRange);
    }
    if (weapon == "weapon_shotgun") {
        return FireShotgun(world, origin, forwardN);
    }
    if (weapon == "weapon_lightning") {
        return FireSingleRay(world, weapon, origin, forwardN, 8.0f);
    }
    if (weapon == "weapon_railgun") {
        return FireRailgun(world, origin, forwardN);
    }
    return {};
}

}  // namespace sdl3cpp::q3
