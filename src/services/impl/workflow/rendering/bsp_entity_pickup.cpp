#include "services/interfaces/workflow/rendering/bsp_entity_update_helpers.hpp"

namespace sdl3cpp::services::impl {

bool ReadVec3(const nlohmann::json& value, btVector3& out) {
    if (!value.is_array() || value.size() != 3) return false;
    out = btVector3(value[0].get<float>(), value[1].get<float>(),
                    value[2].get<float>());
    return true;
}

}  // namespace sdl3cpp::services::impl
