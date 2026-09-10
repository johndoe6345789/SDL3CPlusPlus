#include "services/interfaces/workflow/geometry/geometry_create_cube_helpers.hpp"
#include "services/interfaces/workflow/geometry/cube_geometry_data.hpp"

namespace sdl3cpp::services::impl {

namespace {

constexpr uint8_t kCornerColors[8][3] = {
    {0, 0, 0},        // Black
    {255, 0, 0},      // Red
    {0, 255, 0},      // Green
    {255, 255, 0},    // Yellow
    {0, 0, 255},      // Blue
    {255, 0, 255},    // Magenta
    {0, 255, 255},    // Cyan
    {255, 255, 255},  // White
};

}  // namespace

std::array<PosColorVertex, 8> BuildRainbowCubeVertices() {
    std::array<PosColorVertex, 8> out{};
    for (int i = 0; i < 8; ++i) {
        out[i] = {kCubeCorners[i].x,
                  kCubeCorners[i].y,
                  kCubeCorners[i].z,
                  kCornerColors[i][0],
                  kCornerColors[i][1],
                  kCornerColors[i][2],
                  255};
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
