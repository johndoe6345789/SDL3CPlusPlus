#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cstdint>

namespace sdl3cpp::services::impl {

/// geometry.cube.generate's single solid color, 0-255 per channel.
struct CubeColorParams {
    uint8_t r = 255, g = 255, b = 255;
};

CubeColorParams ReadCubeColorParams(const WorkflowStepDefinition& step);

struct PosColorVertex {
    float x, y, z;
    uint8_t r, g, b, a;
};

/// All 8 corners of kCubeCorners (cube_geometry_data.hpp) in one color.
std::array<PosColorVertex, 8> BuildSolidColorCubeVertices(
    const CubeColorParams& color);

/// Flattens `vertices` into a JSON array of raw bytes -- graphics.
/// buffer.upload reinterprets this back into GPU memory.
nlohmann::json BuildCubeVertexJson(
    const std::array<PosColorVertex, 8>& vertices);

/// kCubeIndices (cube_geometry_data.hpp) as a JSON array of uint16
/// values.
nlohmann::json BuildCubeIndexJson();

}  // namespace sdl3cpp::services::impl
