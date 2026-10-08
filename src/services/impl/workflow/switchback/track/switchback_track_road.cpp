#include "services/interfaces/workflow/switchback/track/switchback_track_road.hpp"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace sdl3cpp::services::impl {
namespace {

std::vector<glm::vec2> SpiralPlan(const SwitchbackRoadSpec& road) {
    std::vector<glm::vec2> flat;
    const float start = glm::radians(road.startAngleDegrees);
    const float sweep = road.turns * glm::two_pi<float>();
    const int samples = std::max(1, road.samples);
    flat.reserve(static_cast<std::size_t>(samples) + 1);
    for (int i = 0; i <= samples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(samples);
        const float theta = start + sweep * t;
        const float r = road.outerRadiusM +
                        (road.innerRadiusM - road.outerRadiusM) * t;
        flat.emplace_back(r * std::cos(theta), r * std::sin(theta));
    }
    return flat;
}

}  // namespace

std::vector<SwitchbackRoadPoint> BuildRoadCentreline(
    const SwitchbackRoadSpec& road) {
    const std::vector<glm::vec2> flat = SpiralPlan(road);
    std::vector<SwitchbackRoadPoint> points;
    points.reserve(flat.size());
    float along = 0.f;
    for (std::size_t i = 0; i < flat.size(); ++i) {
        if (i > 0) along += glm::distance(flat[i - 1], flat[i]);
        points.push_back({glm::vec3(flat[i].x, 0.f, flat[i].y), along});
    }
    const float span = road.endHeightM - road.startHeightM;
    for (SwitchbackRoadPoint& point : points) {
        const float fraction = along > 0.f ? point.alongM / along : 0.f;
        point.position.y = road.startHeightM + span * fraction;
    }
    return points;
}

float MaxGradePercent(const std::vector<SwitchbackRoadPoint>& points) {
    float worst = 0.f;
    for (std::size_t i = 1; i < points.size(); ++i) {
        const float run = points[i].alongM - points[i - 1].alongM;
        if (run <= 0.f) continue;
        const float rise = points[i].position.y - points[i - 1].position.y;
        worst = std::max(worst, std::abs(rise) / run * 100.f);
    }
    return worst;
}

}  // namespace sdl3cpp::services::impl
