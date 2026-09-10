#include "services/interfaces/workflow/gta5/gta5_grid.hpp"

#include <cmath>
#include <cstddef>

namespace sdl3cpp::services::impl {
namespace {

/// A zero tile size in a malformed config divides every position to
/// infinity, and the streamer then silently loads nothing.
float SafeTileSize(const Gta5WorldConfig& world) {
    return world.tileSize > 0.f ? world.tileSize : 512.f;
}

}  // namespace

Gta5TileCoord Gta5TileForPosition(const Gta5WorldConfig& world,
                                  const glm::vec3& position) {
    const float size = SafeTileSize(world);
    Gta5TileCoord tile;
    tile.x = static_cast<int>(
        std::floor((position.x - world.gridOrigin.x) / size));
    tile.z = static_cast<int>(
        std::floor((position.z - world.gridOrigin.y) / size));
    return tile;
}

glm::vec3 Gta5TileCentre(const Gta5WorldConfig& world,
                         const Gta5TileCoord& tile) {
    const float size = SafeTileSize(world);
    return glm::vec3(
        world.gridOrigin.x + (static_cast<float>(tile.x) + 0.5f) * size, 0.f,
        world.gridOrigin.y + (static_cast<float>(tile.z) + 0.5f) * size);
}

Gta5Lod Gta5BandForDistance(const Gta5WorldConfig& world,
                            float distanceMetres) {
    for (std::size_t i = 0; i < world.lodRingDistances.size(); ++i) {
        const float limit = world.lodRingDistances[i];
        // A negative limit is the unbounded outermost ring.
        if (limit < 0.f || distanceMetres <= limit) {
            return static_cast<Gta5Lod>(i);
        }
    }
    return Gta5Lod::Slod2;
}

}  // namespace sdl3cpp::services::impl
