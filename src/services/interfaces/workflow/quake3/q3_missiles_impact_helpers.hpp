#pragma once

#include "services/interfaces/workflow/quake3/q3_missile_types.hpp"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services::impl {

/// Splash damage at `targetPos`: linear falloff to zero at `m.splashRadius`.
int Q3SplashDamage(const sdl3cpp::q3::Q3Missile& m, const glm::vec3& targetPos);

/// A bot entity's `pos` field as a vec3, or the origin if it has none.
glm::vec3 Q3BotPosition(const nlohmann::json& bot);

/**
 * @brief Raycasts each not-yet-exploded missile from its previous position
 * (per `prevPositions`, falling back to its current origin) to its current
 * position; on a hit, snaps its origin to the hit point and marks it
 * exploded. Updates `prevPositions` for every missile either way. A no-op
 * if `world` is null.
 */
void DetectQ3MissileImpacts(
    btDiscreteDynamicsWorld* world,
    std::vector<sdl3cpp::q3::Q3Missile>& missiles,
    std::unordered_map<uint32_t, glm::vec3>& prevPositions);

/**
 * @brief Applies splash damage for every exploded missile to the player
 * (`pendingDamage`, accumulated) and to each bot in `bots` (accumulated
 * into `botDamage`), then drops that missile's `prevPositions` entry.
 */
void ApplyQ3MissileSplashDamage(
    const std::vector<sdl3cpp::q3::Q3Missile>& missiles,
    const glm::vec3& playerPos, const nlohmann::json& bots,
    nlohmann::json& botDamage, int& pendingDamage,
    std::unordered_map<uint32_t, glm::vec3>& prevPositions);

}  // namespace sdl3cpp::services::impl
