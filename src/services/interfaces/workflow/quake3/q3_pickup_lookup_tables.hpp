#pragma once

#include <string>

namespace sdl3cpp::services::impl {

/// ioq3 default ammo granted per `ammo_*` classname.
int DefaultAmmoAmount(const std::string& cls);

/// Maps an `ammo_*` classname to the weapon key used in q3.player_ammo.
std::string AmmoWeaponKey(const std::string& cls);

/// Default ammo granted when picking up a `weapon_*` entity.
int DefaultWeaponAmmo(const std::string& cls);

}  // namespace sdl3cpp::services::impl
