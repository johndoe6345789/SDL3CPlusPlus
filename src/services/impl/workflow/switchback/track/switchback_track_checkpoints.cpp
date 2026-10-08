#include "services/interfaces/workflow/switchback/track/switchback_track_road.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>

namespace sdl3cpp::services::impl {
namespace {

SwitchbackRoadPoint Interpolate(const SwitchbackRoadPoint& a,
                                const SwitchbackRoadPoint& b, float target) {
    const float run = b.alongM - a.alongM;
    const float f = run > 0.f ? (target - a.alongM) / run : 0.f;
    return {glm::mix(a.position, b.position, f), target};
}

}  // namespace

std::vector<glm::vec3> PickCheckpoints(
    const std::vector<SwitchbackRoadPoint>& points, int count) {
    std::vector<glm::vec3> result;
    if (count < 2 || points.size() < 2) return result;
    const float total = points.back().alongM;
    result.reserve(static_cast<std::size_t>(count));
    for (int k = 0; k < count; ++k) {
        const float target = total * static_cast<float>(k) /
                             static_cast<float>(count - 1);
        const auto next = std::lower_bound(
            points.begin(), points.end(), target,
            [](const SwitchbackRoadPoint& point, float value) {
                return point.alongM < value;
            });
        const auto index = static_cast<std::size_t>(
            std::distance(points.begin(), next));
        const std::size_t i = std::clamp<std::size_t>(index, 1,
                                                      points.size() - 1);
        result.push_back(
            Interpolate(points[i - 1], points[i], target).position);
    }
    return result;
}

}  // namespace sdl3cpp::services::impl
