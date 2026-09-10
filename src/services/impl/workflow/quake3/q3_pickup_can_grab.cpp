#include "services/interfaces/workflow/quake3/q3_pickup_apply.hpp"

#include "services/interfaces/workflow/quake3/q3_pickup_lookup_tables.hpp"
#include "services/interfaces/workflow/quake3/q3_pickup_rules_internal.hpp"

namespace sdl3cpp::services::impl {

bool CanPickUpItem(const std::string& cls, const PickupTouchState& state) {
    namespace rules = pickup_rules;

    if (HasClassPrefix(cls, "weapon_")) {
        return true;  // weapons are always picked up
    }
    if (HasClassPrefix(cls, "ammo_")) {
        return state.ammo.value(AmmoWeaponKey(cls), 0) < rules::kMaxAmmo;
    }
    if (rules::IsArmor(cls)) {
        return state.armor < rules::kMaxHealth * 2;
    }
    if (rules::IsHealth(cls)) {
        const int cap = rules::Overheals(HealthQuantity(cls))
                            ? rules::kMaxHealth * 2
                            : rules::kMaxHealth;
        return state.health < cap;
    }
    return false;
}

}  // namespace sdl3cpp::services::impl
