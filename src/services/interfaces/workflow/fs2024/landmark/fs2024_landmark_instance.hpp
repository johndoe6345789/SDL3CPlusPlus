#pragma once

#include <string>

namespace sdl3cpp::services::impl {

/// One placed landmark: a tile's own share of `landmarks.json`, kept
/// as plain data (its GPU mesh/textures are shared, global data --
/// see Fs2024LandmarkKitGpu -- not owned per tile the way a tile's
/// own ground/buildings are).
struct Fs2024LandmarkInstance {
    std::string model;
    float x = 0.f, z = 0.f;
    float headingDegrees = 0.f;
};

}  // namespace sdl3cpp::services::impl
