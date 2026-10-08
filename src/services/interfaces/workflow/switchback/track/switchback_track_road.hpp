#pragma once

#include "services/interfaces/workflow/switchback/track/switchback_track_spec.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace sdl3cpp::services::impl {

struct SwitchbackRoadPoint {
    glm::vec3 position{0.f};
    /// Distance along the road from its start, in metres.
    float alongM = 0.f;
};

/// Spirals from the outer radius inward, rising from startHeight to endHeight.
std::vector<SwitchbackRoadPoint> BuildRoadCentreline(
    const SwitchbackRoadSpec& road);

/// The steepest rise or fall of the road, in percent.
float MaxGradePercent(const std::vector<SwitchbackRoadPoint>& points);

/// `count` points spread evenly by distance, the first at the start line and
/// the last at the finish. Empty when count is below two.
std::vector<glm::vec3> PickCheckpoints(
    const std::vector<SwitchbackRoadPoint>& points, int count);

}  // namespace sdl3cpp::services::impl
