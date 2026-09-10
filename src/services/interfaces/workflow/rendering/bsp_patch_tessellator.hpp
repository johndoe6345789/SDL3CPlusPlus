#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Tessellates one 3x3 biquadratic Bezier control grid into triangles.
 *
 * Q3 patches are stored as overlapping 3x3 sub-patches; this handles exactly
 * one sub-patch, appending its vertices and indices (already offset to the
 * grid's current size) to the output vectors.
 *
 * @param controlPoints Nine control points, row-major.
 * @param level         Tessellation level; each edge is split into `level`
 *                       segments (level+1 samples).
 * @param scale         Q3 world unit -> engine unit scale.
 * @param gridSize      Lightmap atlas grid dimension (see bsp.load's
 *                       bsp_grid_size), used to place this patch's lightmap UV.
 * @param numLightmaps  Number of lightmap slots baked into the atlas.
 * @param lmIndex       This face's lightmap slot, or negative for none.
 * @param outVertices   Appended to; not cleared.
 * @param outIndices    Appended to; not cleared.
 */
void TessellateBspPatch(const BspVertex controlPoints[9], int level,
                        float scale, int gridSize, int numLightmaps,
                        int lmIndex, std::vector<BspRenderVertex>& outVertices,
                        std::vector<uint32_t>& outIndices);

}  // namespace sdl3cpp::services::impl
