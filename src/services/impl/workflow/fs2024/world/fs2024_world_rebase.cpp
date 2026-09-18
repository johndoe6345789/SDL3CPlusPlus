#include "services/interfaces/workflow/fs2024/world/fs2024_world_rebase.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr double kMaxScaleDrift = 0.01;

}  // namespace

bool Fs2024RebaseDue(float x, float z, float radius) {
    return x * x + z * z > radius * radius;
}

Fs2024Rebase RebaseFs2024Origin(Fs2024GeoOrigin& origin, float x, float z) {
    double lat = 0.0, lon = 0.0;
    Fs2024LatLonOfEngine(origin, x, z, lat, lon);
    const Fs2024GeoOrigin fresh = MakeFs2024GeoOrigin(lat, lon);
    Fs2024Rebase rebase;
    rebase.tilesX = fresh.tileX - origin.tileX;
    rebase.tilesY = fresh.tileY - origin.tileY;
    if (std::abs(fresh.metresPerUnit / origin.metresPerUnit - 1.0) >
        kMaxScaleDrift) {
        origin = fresh;
        glm::vec2 moved;
        Fs2024EngineOfLatLon(origin, lat, lon, moved.x, moved.y);
        rebase.shift = glm::vec2(x, z) - moved;
        rebase.rescaled = true;
        return rebase;
    }
    const float size = origin.TileSize();
    rebase.shift = {static_cast<float>(rebase.tilesX) * size,
                    static_cast<float>(rebase.tilesY) * size};
    origin.tileX = fresh.tileX;
    origin.tileY = fresh.tileY;
    return rebase;
}

}  // namespace sdl3cpp::services::impl
