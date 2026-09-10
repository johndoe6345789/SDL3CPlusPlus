#pragma once

#include "services/interfaces/workflow/rendering/bsp_geometry_lumps.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"

namespace sdl3cpp::services::impl {

/**
 * @brief Tessellates one Bezier patch face (BSP face type 2) into a group.
 *
 * A patch's control grid overlaps in 3x3 sub-patches, each tessellated by
 * TessellateBspPatch(); this extracts the grid and drives that per sub-patch.
 *
 * @return false if the face's declared dimensions are degenerate (< 3x3) and
 *         nothing was appended, true otherwise.
 */
bool AppendBspPatchFace(const BspGeometryLumps& lumps, const BspFace& face,
                        float scale, int gridSize, int numLightmaps,
                        int patchTessLevel, TextureGroup& group);

}  // namespace sdl3cpp::services::impl
