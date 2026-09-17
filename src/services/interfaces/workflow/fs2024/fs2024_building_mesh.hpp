#pragma once

#include "services/interfaces/workflow/fs2024/fs2024_polygon.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Appends one building's walls and flat roof to `vertices`/`indices`
/// (BspRenderVertex, the same vertex format terrain uses -- buildings
/// are drawn through the same pipeline, just with a plain texture and
/// no runway overlay). `footprint` is engine (x, z) metres in any
/// winding; `height` is metres. uv/lightmap fields are left at zero,
/// since a flat-massing building has no texture detail to place them
/// against.
///
/// Winding is normalised internally so the roof's and walls' fronts
/// both match this engine's own convention (as `Fs2024TerrainChunkGpu`
/// triangles do): counter-clockwise seen from outside the surface, in
/// x, z, y-up.
void AppendBuildingMesh(const std::vector<Point2>& footprint, float height,
                        std::vector<sdl3cpp::services::impl::BspRenderVertex>&
                            vertices,
                        std::vector<std::uint32_t>& indices);

}  // namespace sdl3cpp::services::impl
