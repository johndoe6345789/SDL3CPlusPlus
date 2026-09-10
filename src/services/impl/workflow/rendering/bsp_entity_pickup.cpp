#include "services/interfaces/workflow/rendering/bsp_entity_update_helpers.hpp"

namespace sdl3cpp::services::impl {

bool HasPrefix(const std::string& value, const std::string& prefix) {
    return value.rfind(prefix, 0) == 0;
}

bool IsPickupClass(const std::string& classname) {
    return HasPrefix(classname, "weapon_") || HasPrefix(classname, "ammo_") ||
           HasPrefix(classname, "item_") || HasPrefix(classname, "holdable_");
}

bool ReadVec3(const nlohmann::json& value, btVector3& out) {
    if (!value.is_array() || value.size() != 3) return false;
    out = btVector3(value[0].get<float>(), value[1].get<float>(),
                    value[2].get<float>());
    return true;
}

bool TryCollectPickup(const nlohmann::json& ent, const std::string& classname,
                      const std::string& id, const btVector3& playerPos,
                      nlohmann::json& collected, nlohmann::json& inventory,
                      WorkflowContext& context,
                      const std::shared_ptr<ILogger>& logger) {
    if (!IsPickupClass(classname)) return false;
    if (id.empty() || collected.value(id, false)) return true;

    btVector3 itemPos;
    if (!ent.contains("position") || !ReadVec3(ent["position"], itemPos)) {
        return true;
    }
    if ((itemPos - playerPos).length2() > 1.2f * 1.2f) return true;

    collected[id]        = true;
    inventory[classname] = true;
    if (HasPrefix(classname, "weapon_")) {
        context.Set<std::string>("q3.current_weapon", classname);
    }
    if (logger) logger->Info("bsp.entities.update: picked up " + classname);
    return true;
}

}  // namespace sdl3cpp::services::impl
