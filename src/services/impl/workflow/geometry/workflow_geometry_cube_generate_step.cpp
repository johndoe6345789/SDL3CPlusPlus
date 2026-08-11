#include "services/interfaces/workflow/geometry/workflow_geometry_cube_generate_step.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>
#include <utility>
#include <cstdint>

using json = nlohmann::json;

namespace sdl3cpp::services::impl {

WorkflowGeometryCubeGenerateStep::WorkflowGeometryCubeGenerateStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGeometryCubeGenerateStep::GetPluginId() const {
    return "geometry.cube.generate";
}

void WorkflowGeometryCubeGenerateStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    if (logger_) {
        logger_->Trace("WorkflowGeometryCubeGenerateStep", "Execute", "", "Entry");
    }

    // Read color parameters (0-255 range), default to white
    uint8_t color_r = 255;
    uint8_t color_g = 255;
    uint8_t color_b = 255;

    auto it_r = step.parameters.find("color_r");
    if (it_r != step.parameters.end()) {
        color_r = static_cast<uint8_t>(it_r->second.numberValue);
    }

    auto it_g = step.parameters.find("color_g");
    if (it_g != step.parameters.end()) {
        color_g = static_cast<uint8_t>(it_g->second.numberValue);
    }

    auto it_b = step.parameters.find("color_b");
    if (it_b != step.parameters.end()) {
        color_b = static_cast<uint8_t>(it_b->second.numberValue);
    }

    // Cube vertex layout: position (3 floats) + color (4 bytes packed as 1 float)
    // Total stride = 16 bytes per vertex (12 position + 4 color)
    //
    // We store vertices as a flat byte stream represented in JSON.
    // Each vertex is stored as an object with {x,y,z,r,g,b,a} for clarity
    // and ease of consumption by the buffer upload step.

    struct PosColorVertex {
        float x, y, z;
        uint8_t r, g, b, a;
    };

    // 8 corners of a unit cube centered at origin, all sharing the same color
    PosColorVertex vertices[] = {
        {-1.0f,  1.0f,  1.0f, color_r, color_g, color_b, 255},  // 0: left  top    front
        { 1.0f,  1.0f,  1.0f, color_r, color_g, color_b, 255},  // 1: right top    front
        {-1.0f, -1.0f,  1.0f, color_r, color_g, color_b, 255},  // 2: left  bottom front
        { 1.0f, -1.0f,  1.0f, color_r, color_g, color_b, 255},  // 3: right bottom front
        {-1.0f,  1.0f, -1.0f, color_r, color_g, color_b, 255},  // 4: left  top    back
        { 1.0f,  1.0f, -1.0f, color_r, color_g, color_b, 255},  // 5: right top    back
        {-1.0f, -1.0f, -1.0f, color_r, color_g, color_b, 255},  // 6: left  bottom back
        { 1.0f, -1.0f, -1.0f, color_r, color_g, color_b, 255},  // 7: right bottom back
    };

    // 12 triangles = 36 indices (CW winding)
    const uint16_t indices[] = {
        0, 1, 2,  2, 1, 3,  // Front face
        4, 6, 5,  5, 6, 7,  // Back face
        0, 2, 4,  4, 2, 6,  // Left face
        1, 5, 3,  5, 7, 3,  // Right face
        0, 4, 1,  4, 5, 1,  // Top face
        2, 3, 6,  6, 3, 7,  // Bottom face
    };

    constexpr int vertex_count = 8;
    constexpr int index_count = 36;
    constexpr int vertex_stride = static_cast<int>(sizeof(PosColorVertex));

    // Serialize vertex data as a flat JSON array of raw bytes.
    // Each vertex is 16 bytes: 3 floats (12 bytes) + 4 uint8 (4 bytes).
    // The upload step will reinterpret this byte array back into GPU memory.
    json vertex_data = json::array();
    const auto* raw = reinterpret_cast<const uint8_t*>(vertices);
    for (size_t i = 0; i < sizeof(vertices); ++i) {
        vertex_data.push_back(static_cast<int>(raw[i]));
    }

    // Serialize index data as a flat JSON array of uint16 values.
    json index_data = json::array();
    for (int i = 0; i < index_count; ++i) {
        index_data.push_back(static_cast<int>(indices[i]));
    }

    // Store results in context
    context.Set("vertex_data", vertex_data);
    context.Set("index_data", index_data);
    context.Set("vertex_count", vertex_count);
    context.Set("index_count", index_count);
    context.Set("vertex_stride", vertex_stride);

    if (logger_) {
        logger_->Info("WorkflowGeometryCubeGenerateStep: Generated cube mesh ("
                      + std::to_string(vertex_count) + " vertices, "
                      + std::to_string(index_count) + " indices, stride="
                      + std::to_string(vertex_stride) + " bytes, color=("
                      + std::to_string(color_r) + ","
                      + std::to_string(color_g) + ","
                      + std::to_string(color_b) + "))");
    }
}

}  // namespace sdl3cpp::services::impl
