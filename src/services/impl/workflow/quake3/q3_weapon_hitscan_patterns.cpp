#include "services/interfaces/workflow/quake3/q3_weapon_hitscan_internal.hpp"
#include "services/interfaces/workflow/quake3/q3_weapon_stats.hpp"

namespace sdl3cpp::q3::hitscan_detail {

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

}  // namespace sdl3cpp::q3::hitscan_detail
