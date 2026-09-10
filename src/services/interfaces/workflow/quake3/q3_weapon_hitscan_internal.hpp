#pragma once

/// Internal helpers shared by the q3_weapon_hitscan_*.cpp files that
/// together implement FireHitscanWeapon() from q3_weapon_hitscan.hpp. Not
/// part of the public workflow-step API.

#include "services/interfaces/workflow/quake3/q3_weapon_hitscan.hpp"

namespace sdl3cpp::q3::hitscan_detail {

constexpr float kHitscanRange = 120.0f;

/// Perpendicular tangent to a direction vector, for spread patterns.
glm::vec3 Tangent(const glm::vec3& dir);

/// Deterministic-ish jitter in [-1, 1], cheap enough to avoid <random>.
float JitterComponent(int seed);

bool Raycast(btDiscreteDynamicsWorld* world, const btVector3& from,
             const btVector3& to, btVector3& hitPoint);

HitscanFireResult FireSingleRay(btDiscreteDynamicsWorld* world,
                                const std::string& weapon,
                                const glm::vec3& origin,
                                const glm::vec3& forwardN, float range);

HitscanFireResult FireShotgun(btDiscreteDynamicsWorld* world,
                              const glm::vec3& origin,
                              const glm::vec3& forwardN);

HitscanFireResult FireRailgun(btDiscreteDynamicsWorld* world,
                              const glm::vec3& origin,
                              const glm::vec3& forwardN);

}  // namespace sdl3cpp::q3::hitscan_detail
