#include "services/interfaces/workflow/quake3/q3_pickup_classify.hpp"

namespace sdl3cpp::services::impl {

bool HasClassnamePrefix(const std::string& value, const std::string& prefix) {
    return value.rfind(prefix, 0) == 0;
}

bool IsPickup(const std::string& classname) {
    return HasClassnamePrefix(classname, "weapon_") ||
           HasClassnamePrefix(classname, "ammo_") ||
           HasClassnamePrefix(classname, "item_") ||
           HasClassnamePrefix(classname, "holdable_");
}

std::string TextureKeyForClass(const std::string& classname) {
    if (HasClassnamePrefix(classname, "weapon_")) return "q3_pickup_weapon";
    if (HasClassnamePrefix(classname, "ammo_")) return "q3_pickup_ammo";
    if (classname.find("health") != std::string::npos) {
        return "q3_pickup_health";
    }
    if (classname.find("armor") != std::string::npos) {
        return "q3_pickup_armor";
    }
    return "q3_pickup_powerup";
}

bool ReadVec3(const nlohmann::json& value, glm::vec3& out) {
    if (!value.is_array() || value.size() != 3) return false;
    out = glm::vec3(value[0].get<float>(), value[1].get<float>(),
                    value[2].get<float>());
    return true;
}

}  // namespace sdl3cpp::services::impl
