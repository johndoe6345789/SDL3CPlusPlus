#pragma once

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

#include <string>

namespace sdl3cpp::q3 {

/// Result of firing one hitscan weapon: accumulated damage and whether it
/// registered at least one hit (a shotgun/railgun can register several).
struct HitscanFireResult {
    int damage = 0;
    bool hit   = false;
};

/**
 * @brief Resolves one shot of a hitscan weapon (machinegun/shotgun/lightning/
 *        railgun) against the physics world.
 *
 * Each weapon's range, pellet count/spread, and piercing behaviour match
 * ioq3's cg_weapons.c. A weapon not in that set (a projectile weapon or the
 * gauntlet) returns a zero result — callers only invoke this for weapons
 * WeaponInstantHitDamage() reports nonzero damage for.
 *
 * @param origin  World-space firing origin (the camera position).
 * @param forward World-space firing direction; need not be normalised.
 */
HitscanFireResult FireHitscanWeapon(btDiscreteDynamicsWorld* world,
                                    const std::string& weapon,
                                    const glm::vec3& origin,
                                    const glm::vec3& forward);

}  // namespace sdl3cpp::q3
