#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_lod.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

void Visit(const Fs2024TileKey& key, float tileSize, const glm::vec3& viewer,
           const Fs2024LodConfig& config, std::vector<Fs2024TileKey>& out) {
    const float span = Fs2024TileSpan(key.level, tileSize);
    if (key.level >= kFs2024FinestLevel ||
        Fs2024TileDistance(key, tileSize, viewer) >= config.split * span) {
        out.push_back(key);
        return;
    }
    for (int dz = 0; dz < 2; ++dz) {
        for (int dx = 0; dx < 2; ++dx) {
            Visit({key.x * 2 + dx, key.z * 2 + dz, key.level + 1}, tileSize,
                  viewer, config, out);
        }
    }
}

}  // namespace

float Fs2024TileDistance(const Fs2024TileKey& key, float tileSize,
                         const glm::vec3& viewer) {
    const glm::vec3 corner = Fs2024TileCorner(key, tileSize);
    const float span = Fs2024TileSpan(key.level, tileSize);
    const float dx = std::max({corner.x - viewer.x, 0.f,
                               viewer.x - (corner.x + span)});
    const float dz = std::max({corner.z - viewer.z, 0.f,
                               viewer.z - (corner.z + span)});
    return std::sqrt(dx * dx + dz * dz + viewer.y * viewer.y);
}

std::vector<Fs2024TileKey> SelectFs2024Tiles(const glm::vec3& viewer,
                                             float tileSize,
                                             const Fs2024LodConfig& config) {
    const Fs2024TileKey centre =
        Fs2024TileKeyFor(viewer.x, viewer.z, tileSize, kFs2024CoarsestLevel);
    std::vector<Fs2024TileKey> leaves;
    for (int dz = -config.rootRadius; dz <= config.rootRadius; ++dz) {
        for (int dx = -config.rootRadius; dx <= config.rootRadius; ++dx) {
            Visit({centre.x + dx, centre.z + dz, kFs2024CoarsestLevel},
                  tileSize, viewer, config, leaves);
        }
    }
    std::stable_sort(leaves.begin(), leaves.end(),
                     [&](const Fs2024TileKey& a, const Fs2024TileKey& b) {
                         return Fs2024TileDistance(a, tileSize, viewer) <
                                Fs2024TileDistance(b, tileSize, viewer);
                     });
    return leaves;
}

}  // namespace sdl3cpp::services::impl
