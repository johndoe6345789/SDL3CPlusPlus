#pragma once

#include "services/interfaces/workflow/switchback/terrain/switchback_heightmap.hpp"
#include "services/interfaces/workflow/switchback/track/switchback_track_spec.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace sdl3cpp::services::impl {

/// Everything a track needs at runtime, expanded from its spec.
struct SwitchbackTrackLayout {
    SwitchbackHeightmap heightmap;
    /// Distance between height samples, in metres.
    float stepM = 0.f;
    float heightMaxM = 0.f;
    float roadLengthM = 0.f;
    float maxGradePercent = 0.f;
    std::vector<glm::vec3> checkpoints;
};

/// Expands the spec into terrain heights with the road carved in, plus the
/// checkpoints along it. The same spec always gives the same track.
bool GenerateSwitchbackTrack(const SwitchbackTrackSpec& spec,
                             SwitchbackTrackLayout& out);

}  // namespace sdl3cpp::services::impl
