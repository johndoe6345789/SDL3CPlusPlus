#include "services/interfaces/workflow/geometry/geometry_cube_generate_helpers.hpp"
#include "services/interfaces/workflow/geometry/cube_geometry_data.hpp"

namespace sdl3cpp::services::impl {

CubeColorParams ReadCubeColorParams(const WorkflowStepDefinition& step) {
    CubeColorParams out;
    auto it = step.parameters.find("color_r");
    if (it != step.parameters.end()) {
        out.r = static_cast<uint8_t>(it->second.numberValue);
    }
    it = step.parameters.find("color_g");
    if (it != step.parameters.end()) {
        out.g = static_cast<uint8_t>(it->second.numberValue);
    }
    it = step.parameters.find("color_b");
    if (it != step.parameters.end()) {
        out.b = static_cast<uint8_t>(it->second.numberValue);
    }
    return out;
}

std::array<PosColorVertex, 8> BuildSolidColorCubeVertices(
    const CubeColorParams& color) {
    std::array<PosColorVertex, 8> out{};
    for (int i = 0; i < 8; ++i) {
        out[i] = {kCubeCorners[i].x, kCubeCorners[i].y, kCubeCorners[i].z,
                  color.r, color.g, color.b, 255};
    }
    return out;
}

nlohmann::json BuildCubeVertexJson(
    const std::array<PosColorVertex, 8>& vertices) {
    nlohmann::json vertexData = nlohmann::json::array();
    const auto* raw = reinterpret_cast<const uint8_t*>(vertices.data());
    for (size_t i = 0; i < sizeof(vertices); ++i) {
        vertexData.push_back(static_cast<int>(raw[i]));
    }
    return vertexData;
}

nlohmann::json BuildCubeIndexJson() {
    nlohmann::json indexData = nlohmann::json::array();
    for (uint16_t idx : kCubeIndices) {
        indexData.push_back(static_cast<int>(idx));
    }
    return indexData;
}

}  // namespace sdl3cpp::services::impl
