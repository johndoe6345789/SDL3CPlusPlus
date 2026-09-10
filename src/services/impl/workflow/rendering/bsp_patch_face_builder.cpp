#include "services/interfaces/workflow/rendering/bsp_patch_face_builder.hpp"
#include "services/interfaces/workflow/rendering/bsp_patch_tessellator.hpp"

#include <vector>

namespace sdl3cpp::services::impl {

bool AppendBspPatchFace(const BspGeometryLumps& lumps, const BspFace& face,
                        float scale, int gridSize, int numLightmaps,
                        int patchTessLevel, TextureGroup& group) {
    const int patchW = face.size[0];
    const int patchH = face.size[1];
    if (patchW < 3 || patchH < 3) {
        return false;
    }

    std::vector<BspVertex> grid(static_cast<size_t>(patchW) * patchH);
    for (int i = 0; i < patchW * patchH && i < face.n_vertices; ++i) {
        const int srcIdx = face.vertex + i;
        if (srcIdx >= 0 && srcIdx < lumps.numVertices) {
            grid[static_cast<size_t>(i)] = lumps.vertices[srcIdx];
        }
    }

    const int numSubPatchesX = (patchW - 1) / 2;
    const int numSubPatchesY = (patchH - 1) / 2;
    for (int py = 0; py < numSubPatchesY; ++py) {
        for (int px = 0; px < numSubPatchesX; ++px) {
            BspVertex controlPoints[9];
            for (int r = 0; r < 3; ++r) {
                for (int c = 0; c < 3; ++c) {
                    controlPoints[r * 3 + c] = grid[static_cast<size_t>(
                        (py * 2 + r) * patchW + (px * 2 + c))];
                }
            }
            TessellateBspPatch(controlPoints, patchTessLevel, scale, gridSize,
                               numLightmaps, face.lm_index, group.vertices,
                               group.indices);
        }
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
