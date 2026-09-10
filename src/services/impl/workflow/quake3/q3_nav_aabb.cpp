#include "services/interfaces/workflow/quake3/q3_nav_aabb.hpp"

namespace sdl3cpp::services::impl {

NavBuildAabb ComputeNavBuildAabb(const nlohmann::json* spawnPts) {
    NavBuildAabb aabb{glm::vec3(-50.f, -10.f, -50.f),
                      glm::vec3(50.f, 50.f, 50.f)};
    if (!spawnPts || !spawnPts->is_array() || spawnPts->empty()) return aabb;

    glm::vec3 mn(1e9f), mx(-1e9f);
    bool any = false;
    for (const auto& sp : *spawnPts) {
        // Accept either {pos:[x,y,z]} or direct [x,y,z] arrays
        const nlohmann::json* posJ = nullptr;
        if (sp.is_array() && sp.size() >= 3)
            posJ = &sp;
        else if (sp.contains("position") && sp["position"].is_array())
            posJ = &sp["position"];
        else if (sp.contains("pos") && sp["pos"].is_array())
            posJ = &sp["pos"];
        if (!posJ) continue;

        glm::vec3 p((*posJ)[0].get<float>(), (*posJ)[1].get<float>(),
                    (*posJ)[2].get<float>());
        mn  = glm::min(mn, p);
        mx  = glm::max(mx, p);
        any = true;
    }
    if (any) {
        // Expand by 20 units to cover the full playable area around spawns
        constexpr float kPad = 20.f;
        aabb.min             = mn - glm::vec3(kPad, 5.f, kPad);
        aabb.max             = mx + glm::vec3(kPad, 5.f, kPad);
    }
    return aabb;
}

}  // namespace sdl3cpp::services::impl
