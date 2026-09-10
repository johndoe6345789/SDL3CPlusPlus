#include "services/interfaces/workflow/rendering/bsp_entity_classification.hpp"

namespace sdl3cpp::services::impl {

namespace {

bool HasPrefix(const std::string& value, const std::string& prefix) {
    return value.rfind(prefix, 0) == 0;
}

}  // namespace

bool IsPickupClass(const std::string& classname) {
    return HasPrefix(classname, "weapon_") || HasPrefix(classname, "ammo_") ||
           HasPrefix(classname, "item_") || HasPrefix(classname, "holdable_");
}

}  // namespace sdl3cpp::services::impl
