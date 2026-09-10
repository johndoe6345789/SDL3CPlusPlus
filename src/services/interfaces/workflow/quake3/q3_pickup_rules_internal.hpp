#pragma once

/// Shared between the "can this be taken" test and the code that
/// applies a pickup, so the two cannot disagree about the limits.

#include <string>

namespace sdl3cpp::services::impl::pickup_rules {

/// STAT_MAX_HEALTH, and Add_Ammo's ceiling.
inline constexpr int kMaxHealth = 100;
inline constexpr int kMaxAmmo   = 200;

// ioq3 g_items.c respawn delays, in seconds. Mega health uses
// RESPAWN_MEGAHEALTH, which ships equal to the normal health delay.
inline constexpr double kRespawnArmor  = 25.0;
inline constexpr double kRespawnHealth = 35.0;
inline constexpr double kRespawnAmmo   = 40.0;
inline constexpr double kRespawnWeapon = 5.0;  // g_weaponrespawn's default

/// Quake singles out the 5 and 100 health items: only those overheal,
/// and only they may be taken while already above the normal maximum.
inline bool Overheals(int quantity) {
    return quantity == 5 || quantity == 100;
}

inline bool IsHealth(const std::string& cls) {
    return cls.find("item_health") != std::string::npos;
}

inline bool IsArmor(const std::string& cls) {
    return cls.find("item_armor") != std::string::npos;
}

}  // namespace sdl3cpp::services::impl::pickup_rules
