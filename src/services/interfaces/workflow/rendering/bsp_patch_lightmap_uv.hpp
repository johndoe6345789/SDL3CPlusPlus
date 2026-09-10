#pragma once

namespace sdl3cpp::services::impl {

/// Lightmap UV offset/scale for one patch within the shared lightmap atlas.
struct LightmapUv {
    float offsetU = 0.0f, offsetV = 0.0f;
    float scaleU = 1.0f, scaleV = 1.0f;
};

/// Computes the atlas UV rect for lightmap slot `lmIndex` (offset by one
/// slot within the `gridSize` x `gridSize` atlas grid). Falls back to the
/// default (0,0,1,1) UV when `lmIndex` is out of [0, numLightmaps) range.
LightmapUv ComputeLightmapUv(int gridSize, int numLightmaps, int lmIndex);

}  // namespace sdl3cpp::services::impl
