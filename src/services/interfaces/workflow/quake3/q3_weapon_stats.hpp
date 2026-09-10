#pragma once

#include <cstdint>
#include <string>

namespace sdl3cpp::q3 {

/// Frames between shots at 60fps, matching ioq3's per-weapon fire rate.
uint32_t WeaponFireInterval(const std::string& weapon);

/// True for weapons that keep firing while the button is held.
bool IsWeaponAutoFire(const std::string& weapon);

/// Damage per hit (or per pellet, for the shotgun) of a hitscan weapon;
/// 0 for weapons that don't hitscan (projectile weapons, gauntlet).
int WeaponInstantHitDamage(const std::string& weapon);

}  // namespace sdl3cpp::q3
