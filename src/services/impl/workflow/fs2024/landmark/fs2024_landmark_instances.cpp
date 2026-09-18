#include "services/interfaces/workflow/fs2024/landmark/fs2024_tile_landmarks.hpp"

#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_place.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"

namespace sdl3cpp::services::impl {

std::vector<Fs2024LandmarkInstance> PlaceFs2024TileLandmarks(
    const Fs2024World& world, const Fs2024TileKey& key,
    const Fs2024Heightfield& field) {
    std::vector<Fs2024LandmarkInstance> instances;
    const glm::vec3 corner = Fs2024TileCorner(key, world.origin.TileSize());
    int quadX = 0, quadY = 0;
    Fs2024QuadOfKey(world.origin, key, quadX, quadY);
    const int side = 1 << (kFs2024TileLevel - key.level);
    for (int dy = 0; dy < side; ++dy) {
        for (int dx = 0; dx < side; ++dx) {
            const auto found = world.landmarks.find(
                Fs2024QuadId(quadX * side + dx, quadY * side + dy));
            if (found == world.landmarks.end()) continue;
            for (const auto& placement : found->second) {
                float x = 0.f, z = 0.f;
                Fs2024EngineOfLatLon(world.origin, placement.lat,
                                     placement.lon, x, z);
                glm::vec3 at(x - corner.x, 0.f, z - corner.z);
                at.y = Fs2024HeightAt(field, at.x, at.z);
                instances.push_back(
                    {placement.model,
                     Fs2024LandmarkModel(at, placement.headingDegrees,
                                         placement.scale)});
            }
        }
    }
    return instances;
}

}  // namespace sdl3cpp::services::impl
