#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Pointers into a loaded BSP's vertex/face/meshvert/texture lumps.
 *
 * Every pointer aliases `bspData`, so the view is only valid as long as that
 * buffer lives; it exists to avoid re-deriving the same four lump offsets in
 * every step that walks BSP faces.
 */
struct BspGeometryLumps {
    const BspVertex* vertices  = nullptr;
    int numVertices            = 0;
    const BspFace* faces       = nullptr;
    int numFaces               = 0;
    const int32_t* meshVerts   = nullptr;
    const BspTexture* textures = nullptr;
    int numTextures            = 0;
};

/// Builds a BspGeometryLumps view over an already-loaded BSP buffer.
BspGeometryLumps ReadBspGeometryLumps(const std::vector<uint8_t>& bspData);

}  // namespace sdl3cpp::services::impl
