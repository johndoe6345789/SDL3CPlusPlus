#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>

namespace sdl3cpp::services::impl {

/// Publishes one of ioq3's four machinegun flash sounds, chosen by frame
/// (cg_weapons.c), for the workflow to play.
void PublishMachinegunFlashSound(WorkflowContext& context, uint32_t fireFrame);

/**
 * @brief Deducts one round of ammo for `weapon`, if it consumes ammo.
 *
 * Always re-publishes `q3.player_ammo` on success (even unchanged, for
 * weapons that don't consume ammo) — but not on a dry click, matching the
 * step's original early-return-before-write-back behaviour.
 *
 * @return false on a dry click (no ammo left); true otherwise.
 */
bool ConsumeWeaponAmmo(WorkflowContext& context, const std::string& weapon);

/// Reads `camera.state.pos`/`.forward`; false (leaving out params untouched)
/// if either is missing.
bool ReadCameraFireVectors(WorkflowContext& context, glm::vec3& origin,
                           glm::vec3& forward);

/**
 * @brief Fires a hitscan weapon, or appends a Q3Missile for a projectile
 *        weapon, to `q3.missiles`.
 * @return true if the shot registered a hit (hitscan weapons only).
 */
bool ResolveWeaponShot(WorkflowContext& context, const std::string& weapon,
                       const glm::vec3& origin, const glm::vec3& forward,
                       uint32_t nextMissileId, int& pendingDamage);

}  // namespace sdl3cpp::services::impl
