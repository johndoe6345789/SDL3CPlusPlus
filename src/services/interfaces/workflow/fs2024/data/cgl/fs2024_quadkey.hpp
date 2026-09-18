#pragma once

#include <cstdint>
#include <string>

namespace sdl3cpp::fs2024 {

/// A Bing-style quadtree tile: the scheme FS2024's CGL world data is
/// cut on (Web Mercator, level 14 for buildings).
struct QuadTile {
    int x = 0, y = 0, level = 0;
};

QuadTile TileAtLatLon(double lat, double lon, int level);

/// The tile's quad digits, e.g. "03131313113010" -- the first six are
/// the CGL file's own folder/name, the rest index the tile inside it.
std::string QuadKeyOf(const QuadTile& tile);

QuadTile TileOfQuadKey(const std::string& quadKey);

/// A bld vertex's lon/lat. `x`/`y` are the tile's own grid units:
/// 1/16384 of the tile from its centre, +y north (see BldVertex).
void BldVertexToLatLon(const QuadTile& tile, std::int32_t x, std::int32_t y,
                       double& lon, double& lat);

}  // namespace sdl3cpp::fs2024
