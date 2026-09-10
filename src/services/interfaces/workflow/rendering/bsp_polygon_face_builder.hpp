#pragma once

#include "services/interfaces/workflow/rendering/bsp_geometry_lumps.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"

namespace sdl3cpp::services::impl {

/**
 * @brief Appends one polygon/mesh face (BSP face type 1 or 3) to a group.
 *
 * Converts vertices from Q3 Z-up to the engine's Y-up and swaps each
 * triangle's second and third index to fix Q3's clockwise winding.
 *
 * @param lmIndex      This face's lightmap slot, or negative for none.
 */
void AppendBspPolygonFace(const BspGeometryLumps& lumps, const BspFace& face,
                          float scale, int gridSize, int numLightmaps,
                          TextureGroup& group);

}  // namespace sdl3cpp::services::impl
