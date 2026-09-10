#include "services/interfaces/workflow/rendering/workflow_bsp_upload_geometry_step.hpp"
#include "services/interfaces/workflow/rendering/bsp_geometry_upload.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowBspUploadGeometryStep::WorkflowBspUploadGeometryStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBspUploadGeometryStep::GetPluginId() const {
    return "bsp.upload_geometry";
}

void WorkflowBspUploadGeometryStep::Execute(const WorkflowStepDefinition& step,
                                            WorkflowContext& context) {
    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device)
        throw std::runtime_error("bsp.upload_geometry: GPU device not found");

    auto allVertices =
        context.Get<std::shared_ptr<std::vector<BspRenderVertex>>>(
            "bsp_all_vertices", nullptr);
    auto allIndices = context.Get<std::shared_ptr<std::vector<uint32_t>>>(
        "bsp_all_indices", nullptr);
    if (!allVertices || !allIndices)
        throw std::runtime_error(
            "bsp.upload_geometry: geometry data not in context");

    auto bspConfig =
        context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    std::string map_name = bspConfig.value("map_name", std::string("q3dm17"));
    std::string meshName = "bsp_" + map_name;

    const BspGeometryBuffers buffers =
        UploadBspGeometryBuffers(device, *allVertices, *allIndices);

    context.Set<SDL_GPUBuffer*>("plane_" + meshName + "_vb",
                                buffers.vertex_buffer);
    context.Set<SDL_GPUBuffer*>("plane_" + meshName + "_ib",
                                buffers.index_buffer);
    context.Set("plane_" + meshName,
                nlohmann::json{{"vertex_count", allVertices->size()},
                               {"index_count", allIndices->size()},
                               {"stride", 40}});

    if (logger_) {
        logger_->Info("bsp.upload_geometry: '" + meshName + "' uploaded (" +
                      std::to_string(allVertices->size()) + " verts, " +
                      std::to_string(allIndices->size() / 3) + " triangles)");
    }
}

}  // namespace sdl3cpp::services::impl
