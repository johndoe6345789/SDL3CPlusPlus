#include "services/interfaces/workflow/rendering/legacy_mesh_normal.hpp"

namespace sdl3cpp::services::impl {

void ApplyLegacyMeshNormal(const nlohmann::json& node, float normal[3]) {
    if (!node.contains("bb_min") || !node.contains("bb_max")) {
        return;
    }
    auto bbMin = node["bb_min"];
    auto bbMax = node["bb_max"];
    float dx   = bbMax[0].get<float>() - bbMin[0].get<float>();
    float dy   = bbMax[1].get<float>() - bbMin[1].get<float>();
    float dz   = bbMax[2].get<float>() - bbMin[2].get<float>();
    if (dy < dx && dy < dz) {
        normal[0] = 0;
        normal[1] = 1;
        normal[2] = 0;
    } else if (dx < dz) {
        normal[0] = 1;
        normal[1] = 0;
        normal[2] = 0;
    } else {
        normal[0] = 0;
        normal[1] = 0;
        normal[2] = 1;
    }
}

}  // namespace sdl3cpp::services::impl
