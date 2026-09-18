#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_quadkey.hpp"

#include <cmath>
#include <stdexcept>

namespace sdl3cpp::fs2024 {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kGridPerTile = 16384.0;

}  // namespace

QuadTile TileAtLatLon(double lat, double lon, int level) {
    const double sine = std::sin(lat * kPi / 180.0);
    const double u = (lon + 180.0) / 360.0;
    const double v = 0.5 - std::log((1.0 + sine) / (1.0 - sine)) / (4.0 * kPi);
    const double side = static_cast<double>(1 << level);
    return {static_cast<int>(u * side), static_cast<int>(v * side), level};
}

std::string QuadKeyOf(const QuadTile& tile) {
    std::string key;
    for (int bit = tile.level; bit > 0; --bit) {
        const int mask = 1 << (bit - 1);
        int digit = 0;
        if (tile.x & mask) digit += 1;
        if (tile.y & mask) digit += 2;
        key.push_back(static_cast<char>('0' + digit));
    }
    return key;
}

QuadTile TileOfQuadKey(const std::string& quadKey) {
    QuadTile tile{0, 0, static_cast<int>(quadKey.size())};
    for (char digit : quadKey) {
        if (digit < '0' || digit > '3') {
            throw std::runtime_error("quadkey: not a quad digit");
        }
        tile.x = tile.x * 2 + ((digit - '0') & 1);
        tile.y = tile.y * 2 + ((digit - '0') >> 1);
    }
    return tile;
}

void BldVertexToLatLon(const QuadTile& tile, std::int32_t x, std::int32_t y,
                       double& lon, double& lat) {
    const double side = static_cast<double>(1 << tile.level);
    // Origin at the tile's centre, +y north: v (Web Mercator, south-
    // positive) runs the other way.
    const double u = tile.x + 0.5 + x / kGridPerTile;
    const double v = tile.y + 0.5 - y / kGridPerTile;
    lon = u / side * 360.0 - 180.0;
    lat = std::atan(std::sinh(kPi * (1.0 - 2.0 * v / side))) * 180.0 / kPi;
}

}  // namespace sdl3cpp::fs2024
