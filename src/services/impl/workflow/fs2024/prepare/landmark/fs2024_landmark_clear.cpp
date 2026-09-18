#include "services/interfaces/workflow/fs2024/prepare/landmark/fs2024_landmark_clear.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::fs2024 {
namespace {

bool UnderALandmark(const BuildingFootprint& building,
                   const std::vector<LandmarkInstance>& landmarks,
                   const std::vector<float>& radii) {
    for (std::size_t i = 0; i < landmarks.size() && i < radii.size(); ++i) {
        if (radii[i] <= 0.f) continue;
        float x = 0.f, z = 0.f;
        for (const services::impl::Point2& point : building.footprint) {
            x += point.x / building.footprint.size();
            z += point.y / building.footprint.size();
        }
        const float dx = x - landmarks[i].x, dz = z - landmarks[i].z;
        if (dx * dx + dz * dz <= radii[i] * radii[i]) return true;
    }
    return false;
}

}  // namespace

void DropBuildingsUnderLandmarks(
    const std::vector<LandmarkInstance>& landmarks,
    const std::vector<float>& radii,
    std::vector<BuildingFootprint>& buildings) {
    if (landmarks.empty()) return;
    buildings.erase(
        std::remove_if(buildings.begin(), buildings.end(),
                      [&](const BuildingFootprint& building) {
                          return UnderALandmark(building, landmarks, radii);
                      }),
        buildings.end());
}

}  // namespace sdl3cpp::fs2024
