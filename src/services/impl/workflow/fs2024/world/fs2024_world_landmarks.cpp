#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_quadkey.hpp"

namespace sdl3cpp::services::impl {

std::uint64_t Fs2024QuadId(int quadX, int quadY) {
    const auto x = static_cast<std::uint32_t>(quadX);
    const auto y = static_cast<std::uint32_t>(quadY);
    return (static_cast<std::uint64_t>(x) << 32) | y;
}

void BucketFs2024Landmarks(
    Fs2024World& world,
    const std::vector<sdl3cpp::fs2024::LandmarkPlacement>& placements) {
    for (const auto& placement : placements) {
        const auto quad = sdl3cpp::fs2024::TileAtLatLon(
            placement.lat, placement.lon, kFs2024TileLevel);
        world.landmarks[Fs2024QuadId(quad.x, quad.y)].push_back(placement);
    }
}

}  // namespace sdl3cpp::services::impl
