#include "services/interfaces/workflow/rendering/bsp_patch_lightmap_uv.hpp"

namespace sdl3cpp::services::impl {

LightmapUv ComputeLightmapUv(int gridSize, int numLightmaps, int lmIndex) {
    LightmapUv uv;
    uv.scaleU = 1.0f / static_cast<float>(gridSize);
    uv.scaleV = 1.0f / static_cast<float>(gridSize);
    if (lmIndex >= 0 && lmIndex < numLightmaps) {
        const int slot  = lmIndex + 1;
        const int slotX = slot % gridSize;
        const int slotY = slot / gridSize;
        uv.offsetU = static_cast<float>(slotX) / static_cast<float>(gridSize);
        uv.offsetV = static_cast<float>(slotY) / static_cast<float>(gridSize);
    }
    return uv;
}

}  // namespace sdl3cpp::services::impl
