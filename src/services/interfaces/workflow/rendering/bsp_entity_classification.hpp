#pragma once

#include <string>

namespace sdl3cpp::services::impl {

/// True if `classname` is one of Q3's pickup prefixes (weapon_/ammo_/item_/
/// holdable_).
bool IsPickupClass(const std::string& classname);

}  // namespace sdl3cpp::services::impl
