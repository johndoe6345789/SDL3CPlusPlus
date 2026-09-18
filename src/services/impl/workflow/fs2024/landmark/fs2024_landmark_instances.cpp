#include "services/interfaces/workflow/fs2024/landmark/fs2024_tile_landmarks.hpp"

#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_place.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"

namespace sdl3cpp::services::impl {

std::vector<Fs2024LandmarkInstance> PlaceFs2024TileLandmarks(
    const Fs2024World& world, int quadX, int quadY,
    const glm::vec3& tileOffset, const Fs2024Heightfield& field) {
    std::vector<Fs2024LandmarkInstance> instances;
    const auto found = world.landmarks.find(Fs2024QuadId(quadX, quadY));
    if (found == world.landmarks.end()) return instances;
    for (const auto& placement : found->second) {
        float x = 0.f, z = 0.f;
        Fs2024EngineOfLatLon(world.origin, placement.lat, placement.lon, x,
                             z);
        glm::vec3 at(x - tileOffset.x, 0.f, z - tileOffset.z);
        at.y = Fs2024HeightAt(field, at.x, at.z);
        instances.push_back(
            {placement.model, Fs2024LandmarkModel(
                                  at, placement.headingDegrees,
                                  placement.scale)});
    }
    return instances;
}

}  // namespace sdl3cpp::services::impl
