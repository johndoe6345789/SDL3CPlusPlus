#pragma once

#include "services/interfaces/workflow/quake3/q3_pickup_effects.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// True if `s` begins with `prefix`.
bool HasClassPrefix(const std::string& s, const std::string& prefix);

/**
 * @brief Whether the player can take this item at all.
 *
 * ioq3 bg_misc.c BG_CanItemBeGrabbed(): weapons always, ammo below 200,
 * armour below twice max health, and health below the maximum — twice
 * it for the small and mega ones, which are the only two that overheal.
 * Without this an item is consumed and sent away to respawn even when
 * it could give nothing, so walking over a health pack while full
 * destroyed it.
 */
bool CanPickUpItem(const std::string& cls, const PickupTouchState& state);

/// Applies one pickup's effect to `state`, returning its respawn delay
/// in seconds (matching ioq3 defaults).
double ApplyOnePickup(const std::string& cls, PickupTouchState& state);

}  // namespace sdl3cpp::services::impl
