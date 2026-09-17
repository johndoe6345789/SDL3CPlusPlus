#include "services/interfaces/workflow/bl4/bl4_tile_key.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

Bl4TileKey Bl4TileKeyFor(float x, float z, float tileSize) {
    return {static_cast<int>(std::floor(x / tileSize)),
           static_cast<int>(std::floor(z / tileSize))};
}

int Bl4TileRing(const Bl4TileKey& key, const Bl4TileKey& centre) {
    return std::max(std::abs(key.x - centre.x), std::abs(key.z - centre.z));
}

std::string Bl4TileDirectory(const std::string& mapRoot, const Bl4TileKey& key) {
    return mapRoot + "/tiles/" + std::to_string(key.x) + "_" + std::to_string(key.z);
}

}  // namespace sdl3cpp::services::impl
