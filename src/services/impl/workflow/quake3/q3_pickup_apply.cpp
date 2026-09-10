#include "services/interfaces/workflow/quake3/q3_pickup_apply.hpp"

#include "services/interfaces/workflow/quake3/q3_pickup_lookup_tables.hpp"
#include "services/interfaces/workflow/quake3/q3_pickup_rules_internal.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

bool HasClassPrefix(const std::string& s, const std::string& prefix) {
    return s.rfind(prefix, 0) == 0;
}

double ApplyOnePickup(const std::string& cls, PickupTouchState& state) {
    namespace rules = pickup_rules;

    if (rules::IsHealth(cls)) {
        const int quantity = HealthQuantity(cls);
        const int cap = rules::Overheals(quantity) ? rules::kMaxHealth * 2
                                                   : rules::kMaxHealth;
        state.health  = std::min(state.health + quantity, cap);
        return rules::kRespawnHealth;
    }
    if (rules::IsArmor(cls)) {
        state.armor =
            std::min(state.armor + ArmorQuantity(cls), rules::kMaxHealth * 2);
        if (cls == "item_armor_combat") {
            state.armorType = "yellow";
        } else if (cls == "item_armor_body") {
            state.armorType = "red";
        }
        return rules::kRespawnArmor;
    }
    if (HasClassPrefix(cls, "ammo_")) {
        const std::string key = AmmoWeaponKey(cls);
        state.ammo[key] =
            std::min(state.ammo.value(key, 0) + DefaultAmmoAmount(cls),
                     rules::kMaxAmmo);
        return rules::kRespawnAmmo;
    }
    if (HasClassPrefix(cls, "weapon_")) {
        state.inventory[cls] = true;
        // Picking a weapon up tops your ammo up to its quantity; if you
        // already hold that much it is worth a single round.
        const int full    = DefaultWeaponAmmo(cls);
        const int held    = state.ammo.value(cls, 0);
        const int granted = held < full ? full - held : 1;
        state.ammo[cls]   = std::min(held + granted, rules::kMaxAmmo);
        return rules::kRespawnWeapon;
    }
    return rules::kRespawnHealth;
}

}  // namespace sdl3cpp::services::impl
