#include "services/interfaces/workflow/quake3/q3_pickup_position.hpp"

namespace sdl3cpp::services::impl {

bool ReadEntityPosition(const nlohmann::json& ent, glm::vec3& out) {
    // "position" is the entity's origin converted into engine space by
    // bsp.parse_spawn. The "origin" the entity also carries is the raw
    // string out of the BSP, still in Quake's units and Z-up axes:
    // reading that instead put every item hundreds of units from where
    // it is drawn, so nothing could ever be walked into.
    if (!ent.contains("position") || !ent["position"].is_array()) {
        return false;
    }
    const auto& p = ent["position"];
    if (p.size() != 3) {
        return false;
    }
    out = glm::vec3(p[0].get<float>(), p[1].get<float>(), p[2].get<float>());
    return true;
}

}  // namespace sdl3cpp::services::impl
