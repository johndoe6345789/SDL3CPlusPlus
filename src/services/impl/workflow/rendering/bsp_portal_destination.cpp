#include "services/interfaces/workflow/rendering/bsp_portal_destination.hpp"

#include <string>

namespace sdl3cpp::services::impl {
namespace {

bool ReadVec3(const nlohmann::json& value, glm::vec3& out) {
    if (!value.is_array() || value.size() != 3) return false;
    out = glm::vec3(value[0].get<float>(), value[1].get<float>(),
                    value[2].get<float>());
    return true;
}

}  // namespace

bool FindPortalDestination(const nlohmann::json& entities, glm::vec3& out) {
    if (!entities.is_array()) return false;
    for (const auto& ent : entities) {
        if (ent.value("classname", std::string{}) != "trigger_teleport") {
            continue;
        }
        if (ent.contains("target_position") &&
            ReadVec3(ent["target_position"], out)) {
            return true;
        }
    }
    return false;
}

}  // namespace sdl3cpp::services::impl
