#pragma once

#include "services/interfaces/workflow/fs2024/fs2024_polygon.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Appends one building's walls and flat roof (BspRenderVertex, the
/// same vertex format terrain uses -- buildings are drawn through the
/// same pipeline, just with real wall/roof textures from FS2024's own
/// generic material kit and no runway overlay) to separate
/// `wallVertices`/`wallIndices` and `roofVertices`/`roofIndices`, so
/// the two can be drawn with their own textures. `footprint` is engine
/// (x, z) metres in any winding; `height` is metres. UVs tile a wall
/// every `kWallTextureMetres` of its own length/height, and a roof
/// every `kRoofTextureMetres` of its own (x, z) -- a flat, planar
/// projection from above, fine for the shallow/flat roofs a simple
/// extrusion produces. Lightmap uv fields are left at zero, unused by
/// this engine's fs2024 shader.
///
/// Winding is normalised internally so the roof's and walls' fronts
/// both match this engine's own convention (as `Fs2024TerrainChunkGpu`
/// triangles do): counter-clockwise seen from outside the surface, in
/// x, z, y-up.
void AppendBuildingMesh(
    const std::vector<Point2>& footprint, float height,
    std::vector<sdl3cpp::services::impl::BspRenderVertex>& wallVertices,
    std::vector<std::uint32_t>& wallIndices,
    std::vector<sdl3cpp::services::impl::BspRenderVertex>& roofVertices,
    std::vector<std::uint32_t>& roofIndices);

}  // namespace sdl3cpp::services::impl
