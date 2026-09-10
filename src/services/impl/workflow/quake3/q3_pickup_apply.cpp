#include "services/interfaces/workflow/quake3/q3_pickup_apply.hpp"

#include "services/interfaces/workflow/quake3/q3_pickup_lookup_tables.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

// Respawn times (seconds) matching ioq3 defaults.
constexpr double kRespawnArmor  = 25.0;
constexpr double kRespawnHealth = 35.0;
constexpr double kRespawnAmmo   = 40.0;
constexpr double kRespawnWeapon = 30.0;

}  // namespace

bool HasClassPrefix(const std::string& s, const std::string& prefix) {
    return s.rfind(prefix, 0) == 0;
}

double ApplyOnePickup(const std::string& cls, PickupTouchState& state) {
    if (cls.find("item_health") != std::string::npos) {
        if (cls == "item_health") {
            state.health = std::min(state.health + 25, 100);
        } else if (cls == "item_health_large") {
            state.health = std::min(state.health + 50, 100);
        } else if (cls == "item_health_mega") {
            state.health = std::min(state.health + 100, 200);
        } else {
            state.health = std::min(state.health + 25, 100);
        }
        return kRespawnHealth;
    }
    if (cls.find("item_armor") != std::string::npos) {
        if (cls == "item_armor_shard") {
            state.armor = std::min(state.armor + 5, 200);
        } else if (cls == "item_armor_combat") {
            state.armor     = std::min(state.armor + 50, 200);
            state.armorType = "green";
        } else if (cls == "item_armor_body") {
            state.armor     = std::min(state.armor + 100, 200);
            state.armorType = "yellow";
        }
        return kRespawnArmor;
    }
    if (HasClassPrefix(cls, "ammo_")) {
        const std::string weaponKey = AmmoWeaponKey(cls);
        const int current           = state.ammo.value(weaponKey, 0);
        state.ammo[weaponKey]       = current + DefaultAmmoAmount(cls);
        return kRespawnAmmo;
    }
    if (HasClassPrefix(cls, "weapon_")) {
        state.inventory[cls] = true;
        const int current    = state.ammo.value(cls, 0);
        state.ammo[cls]      = current + DefaultWeaponAmmo(cls);
        return kRespawnWeapon;
    }
    return kRespawnHealth;
}

}  // namespace sdl3cpp::services::impl
