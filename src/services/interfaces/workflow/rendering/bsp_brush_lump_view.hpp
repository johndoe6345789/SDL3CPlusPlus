#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Raw pointers into a BSP file's texture/brush/brush-side/plane lumps,
/// resolved once so brush-processing code doesn't repeat the offset math.
struct BspBrushLumpView {
    int numTextures               = 0;
    const BspTexture* textures    = nullptr;
    int numBrushes                = 0;
    const BspBrush* brushes       = nullptr;
    int numBrushSides             = 0;
    const BspBrushSide* brushSides = nullptr;
    const BspPlane* planes        = nullptr;
};

/// Resolves the texture/brush/brush-side/plane lump pointers from raw BSP
/// bytes. `bspData` must outlive the returned view.
BspBrushLumpView ParseBspBrushLumps(const std::vector<uint8_t>& bspData);

}  // namespace sdl3cpp::services::impl
