#pragma once

#include <cstdint>
#include <string>

namespace sdl3cpp::services::impl {

struct SwitchbackMountainSpec {
    float baseRadiusM = 900.f;
    float peakHeightM = 420.f;
    float roughness = 0.35f;
    std::uint64_t seed = 1;
};

struct SwitchbackRoadSpec {
    float widthM = 9.f;
    float startAngleDegrees = 200.f;
    float turns = 2.75f;
    float startHeightM = 12.f;
    float endHeightM = 380.f;
    float maxGradePercent = 16.f;
    float outerRadiusM = 765.f;
    float innerRadiusM = 150.f;
    int samples = 2000;
};

/// A track described by parameters. The game expands it into terrain,
/// road and checkpoints at load time, so no baked heightmap ships.
struct SwitchbackTrackSpec {
    std::string id;
    std::string name;
    int gridSize = 1025;
    float heightMaxM = 512.f;
    float noiseAmplitudeM = 60.f;
    float bankM = 30.f;
    SwitchbackMountainSpec mountain;
    SwitchbackRoadSpec road;
    int checkpointCount = 18;
};

/// Reads a track spec JSON file. Missing numbers keep their defaults.
/// Returns false when the file is missing or is not valid JSON.
bool LoadSwitchbackTrackSpec(const std::string& path,
                             SwitchbackTrackSpec& out);

}  // namespace sdl3cpp::services::impl
