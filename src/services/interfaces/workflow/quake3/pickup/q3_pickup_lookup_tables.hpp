#pragma once

#include <string>

namespace sdl3cpp::services::impl {

/// ioq3 default ammo granted per `ammo_*` classname.
int DefaultAmmoAmount(const std::string& cls);

/// Maps an `ammo_*` classname to the weapon key used in q3.player_ammo.
std::string AmmoWeaponKey(const std::string& cls);

/// Default ammo granted when picking up a `weapon_*` entity, from
/// ioq3's bg_itemlist quantities.
int DefaultWeaponAmmo(const std::string& cls);

/// Health an `item_health*` grants. Quake treats 5 and 100 specially:
/// only those two overheal past the normal maximum.
int HealthQuantity(const std::string& cls);

/// Armour an `item_armor*` grants.
int ArmorQuantity(const std::string& cls);

}  // namespace sdl3cpp::services::impl
