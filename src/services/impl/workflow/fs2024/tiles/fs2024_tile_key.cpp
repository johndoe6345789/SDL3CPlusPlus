#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_key.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

float Fs2024TileSpan(int level, float tileSize) {
    return tileSize * static_cast<float>(1 << (kFs2024FinestLevel - level));
}

Fs2024TileKey Fs2024TileKeyFor(float x, float z, float tileSize,
                               int level) {
    const float span = Fs2024TileSpan(level, tileSize);
    return {static_cast<int>(std::floor(x / span)),
            static_cast<int>(std::floor(z / span)), level};
}

glm::vec3 Fs2024TileCorner(const Fs2024TileKey& key, float tileSize) {
    const float span = Fs2024TileSpan(key.level, tileSize);
    return {static_cast<float>(key.x) * span, 0.f,
            static_cast<float>(key.z) * span};
}

Fs2024TileKey Fs2024TileParent(const Fs2024TileKey& key) {
    // An arithmetic shift floors, so tile -1's parent is -1, not 0.
    return {key.x >> 1, key.z >> 1, key.level - 1};
}

bool Fs2024TileContains(const Fs2024TileKey& outer,
                        const Fs2024TileKey& inner) {
    if (inner.level < outer.level) return false;
    const int shift = inner.level - outer.level;
    return (inner.x >> shift) == outer.x && (inner.z >> shift) == outer.z;
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
