#include "services/interfaces/workflow/gta5/gta5_lights.hpp"

namespace sdl3cpp::services::impl {

std::vector<BspRenderVertex> BuildGta5LightSprites(
    const std::vector<Gta5DistantLight>& lights) {
    constexpr float kCorners[6][2] = {{-1.f, -1.f}, {1.f, -1.f}, {1.f, 1.f},
                                      {-1.f, -1.f}, {1.f, 1.f},  {-1.f, 1.f}};
    std::vector<BspRenderVertex> out;
    out.reserve(lights.size() * 6);
    for (const Gta5DistantLight& light : lights) {
        for (const auto& corner : kCorners) {
            BspRenderVertex v{};
            v.x = light.position.x, v.y = light.position.y;
            v.z = light.position.z;
            v.u = corner[0], v.v = corner[1];
            v.nx = light.colour.r, v.ny = light.colour.g;
            v.nz = light.colour.b;
            v.lm_u = light.intensity;
            out.push_back(v);
        }
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
