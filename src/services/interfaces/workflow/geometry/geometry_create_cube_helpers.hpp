#pragma once

#include <array>
#include <cstdint>

namespace sdl3cpp::services::impl {

/// Cube vertex structure: position (xyz) + color (RGBA as 4 normalized
/// bytes). Layout matches pipeline: Float3 position at offset 0,
/// UByte4Norm color at offset 12.
struct PosColorVertex {
    float x, y, z;
    uint8_t r, g, b, a;
};

/// The debug cube's 8 corners (kCubeCorners), each given its own color
/// so every face is visibly striped.
std::array<PosColorVertex, 8> BuildRainbowCubeVertices();

}  // namespace sdl3cpp::services::impl
