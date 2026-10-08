#include "services/interfaces/workflow/switchback/track/switchback_track_carve.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace sdl3cpp::services::impl {
namespace {

float Smooth01(float t) {
    const float clamped = std::clamp(t, 0.f, 1.f);
    return clamped * clamped * (3.f - 2.f * clamped);
}

int CellIndex(float metres, float extentM, float stepM) {
    return static_cast<int>(std::floor((metres + extentM) / stepM));
}

void CarveAround(SwitchbackHeightmap& map, float stepM, float extentM,
                 const SwitchbackRoadPoint& point, float roadHalfWidthM,
                 float bankM) {
    const int last = map.size - 1;
    const float x = point.position.x;
    const float z = point.position.z;
    const int loI = std::max(0, CellIndex(x - bankM, extentM, stepM));
    const int hiI = std::min(last, CellIndex(x + bankM, extentM, stepM) + 1);
    const int loJ = std::max(0, CellIndex(z - bankM, extentM, stepM));
    const int hiJ = std::min(last, CellIndex(z + bankM, extentM, stepM) + 1);
    for (int j = loJ; j <= hiJ; ++j) {
        const float sampleZ = -extentM + static_cast<float>(j) * stepM;
        for (int i = loI; i <= hiI; ++i) {
            const float sampleX = -extentM + static_cast<float>(i) * stepM;
            const float distance = std::hypot(sampleX - x, sampleZ - z);
            if (distance >= bankM) continue;
            const float t = (distance - roadHalfWidthM) /
                            (bankM - roadHalfWidthM);
            const float weight = 1.f - Smooth01(t);
            float& height = map.metres[static_cast<std::size_t>(j) *
                                           static_cast<std::size_t>(map.size) +
                                       static_cast<std::size_t>(i)];
            height += (point.position.y - height) * weight;
        }
    }
}

}  // namespace

void CarveRoadIntoHeights(SwitchbackHeightmap& map, float stepM, float extentM,
                          const std::vector<SwitchbackRoadPoint>& points,
                          float roadHalfWidthM, float bankM) {
    for (const SwitchbackRoadPoint& point : points) {
        CarveAround(map, stepM, extentM, point, roadHalfWidthM, bankM);
    }
}

}  // namespace sdl3cpp::services::impl
