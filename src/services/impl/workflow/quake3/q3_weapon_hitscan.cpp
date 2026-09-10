#include "services/interfaces/workflow/quake3/q3_weapon_hitscan_internal.hpp"

namespace sdl3cpp::q3 {

HitscanFireResult FireHitscanWeapon(btDiscreteDynamicsWorld* world,
                                    const std::string& weapon,
                                    const glm::vec3& origin,
                                    const glm::vec3& forward) {
    namespace detail = hitscan_detail;
    const glm::vec3 forwardN = glm::normalize(forward);

    if (weapon == "weapon_machinegun") {
        return detail::FireSingleRay(world, weapon, origin, forwardN,
                                     detail::kHitscanRange);
    }
    if (weapon == "weapon_shotgun") {
        return detail::FireShotgun(world, origin, forwardN);
    }
    if (weapon == "weapon_lightning") {
        return detail::FireSingleRay(world, weapon, origin, forwardN, 8.0f);
    }
    if (weapon == "weapon_railgun") {
        return detail::FireRailgun(world, origin, forwardN);
    }
    return {};
}

}  // namespace sdl3cpp::q3
