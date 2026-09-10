#pragma once

#include <cstdint>

namespace sdl3cpp::services::impl {

/// The 8 corners of a unit cube centered at the origin, shared by
/// geometry.create_cube and geometry.cube.generate so both draw the
/// same shape (only per-vertex coloring differs between them).
struct CubeCorner {
    float x, y, z;
};

inline constexpr CubeCorner kCubeCorners[8] = {
    {-1.0f, 1.0f, 1.0f},    // 0: left  top    front
    {1.0f, 1.0f, 1.0f},     // 1: right top    front
    {-1.0f, -1.0f, 1.0f},   // 2: left  bottom front
    {1.0f, -1.0f, 1.0f},    // 3: right bottom front
    {-1.0f, 1.0f, -1.0f},   // 4: left  top    back
    {1.0f, 1.0f, -1.0f},    // 5: right top    back
    {-1.0f, -1.0f, -1.0f},  // 6: left  bottom back
    {1.0f, -1.0f, -1.0f},   // 7: right bottom back
};

/// 12 triangles = 36 indices (CW winding) for kCubeCorners.
inline constexpr uint16_t kCubeIndices[36] = {
    0, 1, 2, 2, 1, 3,  // Front face
    4, 6, 5, 5, 6, 7,  // Back face
    0, 2, 4, 4, 2, 6,  // Left face
    1, 5, 3, 5, 7, 3,  // Right face
    0, 4, 1, 4, 5, 1,  // Top face
    2, 3, 6, 6, 3, 7,  // Bottom face
};

}  // namespace sdl3cpp::services::impl
