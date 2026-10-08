#include "services/interfaces/workflow/switchback/track/switchback_track_generate.hpp"

#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_noise.hpp"
#include "services/interfaces/workflow/switchback/track/switchback_track_carve.hpp"
#include "services/interfaces/workflow/switchback/track/switchback_track_road.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace sdl3cpp::services::impl {
namespace {

float MountainHeight(float radius, float baseRadius, float peak) {
    const float t = std::min(radius / baseRadius, 1.f);
    return peak * std::pow(1.f - t * t, 1.5f);
}

std::vector<float> BuildGroundHeights(const SwitchbackTrackSpec& spec,
                                      float stepM) {
    const int size = spec.gridSize;
    const float base = spec.mountain.baseRadiusM;
    const SwitchbackTerrainNoise noise(spec.mountain.seed);
    const float noiseScale = spec.noiseAmplitudeM * spec.mountain.roughness;
    const float span = static_cast<float>(size - 1);
    const auto stride = static_cast<std::size_t>(size);
    std::vector<float> heights(stride * stride);
    for (int j = 0; j < size; ++j) {
        const float z = -base + static_cast<float>(j) * stepM;
        for (int i = 0; i < size; ++i) {
            const float x = -base + static_cast<float>(i) * stepM;
            float height = MountainHeight(std::hypot(x, z), base,
                                          spec.mountain.peakHeightM);
            height += noiseScale * noise.Sample(static_cast<float>(i) / span,
                                                static_cast<float>(j) / span);
            heights[static_cast<std::size_t>(j) * stride +
                    static_cast<std::size_t>(i)] =
                std::clamp(height, 0.f, spec.heightMaxM);
        }
    }
    return heights;
}

}  // namespace

bool GenerateSwitchbackTrack(const SwitchbackTrackSpec& spec,
                             SwitchbackTrackLayout& out) {
    if (spec.gridSize < 2 || spec.mountain.baseRadiusM <= 0.f) return false;
    const float base = spec.mountain.baseRadiusM;
    const float stepM = 2.f * base / static_cast<float>(spec.gridSize - 1);
    const std::vector<SwitchbackRoadPoint> road =
        BuildRoadCentreline(spec.road);

    out.stepM = stepM;
    out.heightMaxM = spec.heightMaxM;
    out.heightmap.size = spec.gridSize;
    out.heightmap.metres = BuildGroundHeights(spec, stepM);
    CarveRoadIntoHeights(out.heightmap, stepM, base, road,
                         spec.road.widthM / 2.f, spec.bankM);
    for (float& height : out.heightmap.metres) {
        height = std::clamp(height, 0.f, spec.heightMaxM);
    }
    out.roadLengthM = road.empty() ? 0.f : road.back().alongM;
    out.maxGradePercent = MaxGradePercent(road);
    out.checkpoints = PickCheckpoints(road, spec.checkpointCount);
    return !out.checkpoints.empty();
}

}  // namespace sdl3cpp::services::impl
