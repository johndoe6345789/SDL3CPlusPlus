#include "services/interfaces/workflow/geometry/workflow_geometry_create_plane_step.hpp"
#include "services/interfaces/workflow/geometry/geometry_plane_helpers.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowGeometryCreatePlaneStep::WorkflowGeometryCreatePlaneStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGeometryCreatePlaneStep::GetPluginId() const {
    return "geometry.create_plane";
}

void WorkflowGeometryCreatePlaneStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    const GeometryPlaneParams params = ReadGeometryPlaneParams(step);
    const GeometryPlaneMesh mesh     = BuildGeometryPlaneMesh(params);

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error(
            "geometry.create_plane: GPU device not found in context");
    }

    const GeometryPlaneBuffers buffers = UploadGeometryPlaneMesh(device, mesh);

    context.Set<SDL_GPUBuffer*>("plane_" + params.name + "_vb",
                                buffers.vertexBuffer);
    context.Set<SDL_GPUBuffer*>("plane_" + params.name + "_ib",
                                buffers.indexBuffer);

    const auto vertexCount = static_cast<uint32_t>(mesh.vertices.size());
    const auto indexCount  = static_cast<uint32_t>(mesh.indices.size());
    nlohmann::json meta    = {{"vertex_count", vertexCount},
                              {"index_count", indexCount},
                              {"stride", 20},
                              {"width", params.width},
                              {"depth", params.depth},
                              {"subdivisions_x", params.subdivisionsX},
                              {"subdivisions_y", params.subdivisionsY}};
    context.Set("plane_" + params.name, meta);

    if (logger_) {
        logger_->Info("geometry.create_plane: '" + params.name + "' created (" +
                      std::to_string(vertexCount) + " verts, " +
                      std::to_string(indexCount) + " indices, " +
                      std::to_string(params.subdivisionsX) + "x" +
                      std::to_string(params.subdivisionsY) + " subdivisions)");
    }
}

}  // namespace sdl3cpp::services::impl
