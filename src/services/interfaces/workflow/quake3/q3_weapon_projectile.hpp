#pragma once

#include "services/interfaces/workflow/quake3/q3_missile_types.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>

namespace sdl3cpp::q3 {

/**
 * @brief Builds the Q3Missile a projectile weapon fires, per ioq3's stats.
 *
 * @return true and fills `out` for rocket/grenade/plasma/bfg launchers;
 *         false (leaving `out` unmodified) for any other weapon, including
 *         hitscan weapons and the gauntlet — see FireHitscanWeapon() for
 *         those instead.
 */
bool BuildWeaponProjectile(const std::string& weapon, const glm::vec3& origin,
                           const glm::vec3& forward, uint32_t id,
                           Q3Missile& out);

}  // namespace sdl3cpp::q3
