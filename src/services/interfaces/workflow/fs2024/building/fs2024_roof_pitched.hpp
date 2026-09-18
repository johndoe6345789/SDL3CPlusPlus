#pragma once

#include "services/interfaces/workflow/fs2024/building/fs2024_oriented_box.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Builds the six-point solid every pitched roof shares: four eave
/// corners of `box` at `eaveY` and a ridge `rise` metres above it,
/// `ridgeHalf` metres either side of the centre along the box's long
/// axis. `ridgeHalf` equal to the box's half length gives a gable, a
/// shorter one a hip, zero a pyramid. Appends to `vertices`/`indices` as BspRenderVertex.
void AppendPitchedRoof(const OrientedBox& box, float ridgeHalf, float eaveY,
                       float rise, float metresPerTile,
                       std::vector<BspRenderVertex>& vertices,
                       std::vector<std::uint32_t>& indices);

}  // namespace sdl3cpp::services::impl
