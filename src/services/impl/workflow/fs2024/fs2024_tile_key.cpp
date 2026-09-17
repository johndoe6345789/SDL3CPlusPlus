#include "services/interfaces/workflow/fs2024/fs2024_tile_key.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

Fs2024TileKey Fs2024TileKeyFor(float x, float z, float tileSize) {
    return {static_cast<int>(std::floor(x / tileSize)),
           static_cast<int>(std::floor(z / tileSize))};
}

int Fs2024TileRing(const Fs2024TileKey& key, const Fs2024TileKey& centre) {
    return std::max(std::abs(key.x - centre.x), std::abs(key.z - centre.z));
}

std::string Fs2024TileDirectory(const std::string& tilesRoot,
                                const Fs2024TileKey& key) {
    return tilesRoot + "/tiles/" + std::to_string(key.x) + "_" +
          std::to_string(key.z);
}

}  // namespace sdl3cpp::services::impl
