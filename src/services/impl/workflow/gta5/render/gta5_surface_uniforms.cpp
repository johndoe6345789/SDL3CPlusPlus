#include "services/interfaces/workflow/gta5/render/gta5_draw_bind.hpp"

#include <cstring>

namespace sdl3cpp::services::impl {

bool SetGta5SurfaceUniforms(rendering::FragmentUniformData& fu,
                            const Gta5SubMesh& sub) {
    // Terrain uses its layers as layers, not as normal and specular maps.
    const bool lit      = !sub.terrain;
    const float maps[2] = {lit && sub.layers[0] ? 1.f : 0.f,
                           lit && sub.layers[1] ? 1.f : 0.f};
    const bool same = std::memcmp(fu.flash_color, sub.surface.data(),
                                  sizeof(fu.flash_color)) == 0 &&
                      std::memcmp(fu.material, maps, sizeof(maps)) == 0;
    if (same) return false;
    std::memcpy(fu.flash_color, sub.surface.data(), sizeof(fu.flash_color));
    std::memcpy(fu.material, maps, sizeof(maps));
    return true;
}

}  // namespace sdl3cpp::services::impl
