#pragma once

#include "services/interfaces/workflow/fs2024/building/fs2024_polygon.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// The roof shapes FS2024's own building generator distributes (its
/// `roof_type` rule key: Flat, Gabled, Hipped, Pyramidal).
enum class RoofShape { Flat, Gabled, Hipped, Pyramidal };

/// Appends a roof over `footprint` at eave height `eaveY`, rising
/// `rise` metres to its ridge, to `vertices`/`indices` (the same
/// BspRenderVertex the walls use, so roofs draw in the roof-texture
/// pass). Pitched shapes are built over the footprint's own oriented
/// bounding box -- a real terrace or semi has its ridge along its
/// long axis -- and the gable ends are filled so no hole is left
/// above the wall. UVs tile every `metresPerTile` metres of roof.
/// Returns the height of the ridge above `eaveY` actually used (zero
/// for a flat roof).
float AppendRoofMesh(const std::vector<Point2>& footprint, float eaveY,
                     RoofShape shape, float rise, float metresPerTile,
                     std::vector<BspRenderVertex>& vertices,
                     std::vector<std::uint32_t>& indices);

}  // namespace sdl3cpp::services::impl
