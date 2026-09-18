#include "services/interfaces/workflow/fs2024/prepare/cgl/fs2024_bld_buildings.hpp"

#include "services/interfaces/workflow/fs2024/building/fs2024_oriented_box.hpp"
#include "services/interfaces/workflow/fs2024/prepare/cgl/fs2024_bld_convert.hpp"
#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_library.hpp"

#include <algorithm>
#include <set>

namespace sdl3cpp::fs2024 {
namespace {

constexpr int kBldLevel = 14;

/// The level-14 tiles covering a square `extent` metres on a side.
std::vector<QuadTile> TilesCovering(const LocalFrame& frame, float extent) {
    const float half = extent / 2.f;
    double west = 0, east = 0, north = 0, south = 0, ignored = 0;
    frame.ToGeo(-half, -half, west, north);
    frame.ToGeo(half, half, east, south);
    frame.ToGeo(-half, half, ignored, south);
    const QuadTile topLeft = TileAtLatLon(north, west, kBldLevel);
    const QuadTile bottomRight = TileAtLatLon(south, east, kBldLevel);
    std::vector<QuadTile> tiles;
    for (int y = topLeft.y; y <= bottomRight.y; ++y) {
        for (int x = topLeft.x; x <= bottomRight.x; ++x) {
            tiles.push_back({x, y, kBldLevel});
        }
    }
    return tiles;
}

}  // namespace

std::vector<BuildingFootprint> ReadFs2024Buildings(
    const std::string& cglRoot, const LocalFrame& frame, float extent) {
    BldLibrary library(cglRoot);
    std::vector<BuildingFootprint> buildings;
    for (const QuadTile& tile : TilesCovering(frame, extent)) {
        bool surveyedFirst = true;
        std::vector<BuildingFootprint> fromTile;
        for (const BldTile& decoded : library.ReadTile(tile)) {
            AppendBldBuildings(decoded, tile, frame, extent, surveyedFirst,
                              fromTile);
            surveyedFirst = false;  // the imagery set only fills gaps
        }
        buildings.insert(buildings.end(), fromTile.begin(), fromTile.end());
    }
    return buildings;
}

}  // namespace sdl3cpp::fs2024
