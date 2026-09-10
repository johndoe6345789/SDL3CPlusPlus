#include "services/interfaces/workflow/geometry/workflow_geometry_cube_generate_step.hpp"
#include "services/interfaces/workflow/geometry/geometry_cube_generate_helpers.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGeometryCubeGenerateStep::WorkflowGeometryCubeGenerateStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGeometryCubeGenerateStep::GetPluginId() const {
    return "geometry.cube.generate";
}

void WorkflowGeometryCubeGenerateStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowGeometryCubeGenerateStep", "Execute", "",
                       "Entry");
    }

    const CubeColorParams color = ReadCubeColorParams(step);
    const auto vertices         = BuildSolidColorCubeVertices(color);
    constexpr int kVertexCount  = 8;
    constexpr int kIndexCount   = 36;
    constexpr int kVertexStride = static_cast<int>(sizeof(PosColorVertex));

    context.Set("vertex_data", BuildCubeVertexJson(vertices));
    context.Set("index_data", BuildCubeIndexJson());
    context.Set("vertex_count", kVertexCount);
    context.Set("index_count", kIndexCount);
    context.Set("vertex_stride", kVertexStride);

    if (logger_) {
        logger_->Info(
            "WorkflowGeometryCubeGenerateStep: Generated cube mesh (" +
            std::to_string(kVertexCount) + " vertices, " +
            std::to_string(kIndexCount) +
            " indices, stride=" + std::to_string(kVertexStride) +
            " bytes, color=(" + std::to_string(color.r) + "," +
            std::to_string(color.g) + "," + std::to_string(color.b) + "))");
    }
}

}  // namespace sdl3cpp::services::impl
