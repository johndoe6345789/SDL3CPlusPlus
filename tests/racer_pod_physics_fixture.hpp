#pragma once

#include "services/interfaces/workflow/racer/player/racer_pod_physics.hpp"

using sdl3cpp::services::impl::RacerPodInput;
using sdl3cpp::services::impl::RacerPodSpec;
using sdl3cpp::services::impl::RacerPodState;
using sdl3cpp::services::impl::RacerSurface;
using sdl3cpp::services::impl::StepRacerPod;

namespace racer_test {

/// A flat floor at height 0 everywhere, below any ceiling above it.
inline std::optional<float> Flat(const void*, float, float, float ceiling) {
    return ceiling >= 0.f ? std::optional<float>(0.f) : std::nullopt;
}

/// A floor only where z > -50: beyond that, a wall up to height 5.
inline std::optional<float> Walled(const void*, float, float z, float ceiling) {
    if (z > -50.f) return ceiling >= 0.f ? std::optional<float>(0.f)
                                         : std::nullopt;
    return ceiling >= 5.f ? std::optional<float>(5.f) : std::nullopt;
}

inline std::optional<float> Gap(const void*, float, float, float) {
    return std::nullopt;
}

/// A wall across the course at z = -30.
inline bool WallAt30(const void*, const glm::vec3& from, const glm::vec3& to) {
    return (from.z + 30.f) * (to.z + 30.f) <= 0.f;
}

inline RacerSurface Surface(decltype(RacerSurface::height) height,
                     decltype(RacerSurface::wall) wall = nullptr) {
    RacerSurface surface;
    surface.height = height;
    surface.wall = wall;
    return surface;
}

inline RacerPodState Fly(RacerPodInput input, float seconds,
                  const RacerSurface& surface) {
    RacerPodState pod;
    const RacerPodSpec spec;
    for (float t = 0.f; t < seconds; t += 1.f / 60.f) {
        StepRacerPod(pod, input, spec, 1.f / 60.f, surface);
    }
    return pod;
}

}  // namespace racer_test
