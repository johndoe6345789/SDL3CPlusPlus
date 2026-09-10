#include "services/interfaces/workflow/quake3/q3_item_models.hpp"

#include "services/interfaces/workflow/quake3/q3_weapon_cycle.hpp"

namespace sdl3cpp::services::impl {
namespace {

bool HasPrefix(const std::string& value, const std::string& prefix) {
    return value.rfind(prefix, 0) == 0;
}

}  // namespace

std::string Q3ItemModelPrefix(const std::string& classname) {
    if (HasPrefix(classname, "weapon_")) {
        return Q3WeaponModelPrefix(classname);
    }
    if (HasPrefix(classname, "item_armor") ||
        HasPrefix(classname, "item_health") || HasPrefix(classname, "ammo_")) {
        // Loaded under the classname itself by q3_map_session.
        return classname;
    }
    return {};
}

bool Q3ItemSpinsFast(const std::string& classname) {
    return HasPrefix(classname, "item_health");
}

}  // namespace sdl3cpp::services::impl
