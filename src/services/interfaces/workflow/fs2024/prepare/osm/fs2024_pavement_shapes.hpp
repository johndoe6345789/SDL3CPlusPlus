#pragma once

#include "services/interfaces/workflow/fs2024/prepare/bgl/fs2024_bgl_airport.hpp"
#include "services/interfaces/workflow/fs2024/prepare/fs2024_local_frame.hpp"
#include "services/interfaces/workflow/fs2024/prepare/osm/fs2024_osm_json.hpp"

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

namespace sdl3cpp::tools::fs2024 {

struct Shape {
    std::array<std::uint8_t, 3> colour;
    std::vector<std::pair<float, float>> polygon;  ///< engine (x, z)
};

/// A road way's centreline as flat quads, in engine metres: the base
/// colour a road's ground texture gets, with the same "no fine detail
/// to blur" reasoning that already keeps a runway's asphalt flat (its
/// markings are drawn analytically instead -- see fs2024_runway.glsl).
std::vector<Shape> RoadShapes(const std::vector<OsmWay>& roads,
                              const LocalFrame& frame);

/// An airport's apron triangles (already triangulated by the BGL) plus
/// its longest runway's asphalt rectangle, no markings baked in for
/// the same reason.
std::vector<Shape> AirportShapes(const Airport& airport,
                                 const LocalFrame& frame);

}  // namespace sdl3cpp::tools::fs2024
