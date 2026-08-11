#include "services/interfaces/workflow/rendering/workflow_bsp_upload_geometry_step.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowBspUploadGeometryStep::WorkflowBspUploadGeometryStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBspUploadGeometryStep::GetPluginId() const {
    return "bsp.upload_geometry";
}

void WorkflowBspUploadGeometryStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) throw std::runtime_error("bsp.upload_geometry: GPU device not found");

    auto allVertices = context.Get<std::shared_ptr<std::vector<BspRenderVertex>>>("bsp_all_vertices", nullptr);
    auto allIndices = context.Get<std::shared_ptr<std::vector<uint32_t>>>("bsp_all_indices", nullptr);
    if (!allVertices || !allIndices)
        throw std::runtime_error("bsp.upload_geometry: geometry data not in context");

    auto bspConfig = context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    std::string map_name = bspConfig.value("map_name", std::string("q3dm17"));
    std::string meshName = "bsp_" + map_name;

    uint32_t vtxSize = static_cast<uint32_t>(allVertices->size() * sizeof(BspRenderVertex));
    uint32_t idxSize = static_cast<uint32_t>(allIndices->size() * sizeof(uint32_t));

    SDL_GPUBufferCreateInfo vbInfo = {};
    vbInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vbInfo.size = vtxSize;
    SDL_GPUBuffer* vb = SDL_CreateGPUBuffer(device, &vbInfo);

    SDL_GPUBufferCreateInfo ibInfo = {};
    ibInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    ibInfo.size = idxSize;
    SDL_GPUBuffer* ib = SDL_CreateGPUBuffer(device, &ibInfo);

    SDL_GPUTransferBufferCreateInfo tbInfo = {};
    tbInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbInfo.size = vtxSize + idxSize;
    SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device, &tbInfo);

    auto* mapped = static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(device, tb, false));
    std::memcpy(mapped, allVertices->data(), vtxSize);
    std::memcpy(mapped + vtxSize, allIndices->data(), idxSize);
    SDL_UnmapGPUTransferBuffer(device, tb);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* cp = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation srcV = {}; srcV.transfer_buffer = tb;
    SDL_GPUBufferRegion dstV = {}; dstV.buffer = vb; dstV.size = vtxSize;
    SDL_UploadToGPUBuffer(cp, &srcV, &dstV, false);

    SDL_GPUTransferBufferLocation srcI = {}; srcI.transfer_buffer = tb; srcI.offset = vtxSize;
    SDL_GPUBufferRegion dstI = {}; dstI.buffer = ib; dstI.size = idxSize;
    SDL_UploadToGPUBuffer(cp, &srcI, &dstI, false);

    SDL_EndGPUCopyPass(cp);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, tb);

    context.Set<SDL_GPUBuffer*>("plane_" + meshName + "_vb", vb);
    context.Set<SDL_GPUBuffer*>("plane_" + meshName + "_ib", ib);
    context.Set("plane_" + meshName, nlohmann::json{
        {"vertex_count", allVertices->size()},
        {"index_count", allIndices->size()},
        {"stride", 40}
    });

    if (logger_) {
        logger_->Info("bsp.upload_geometry: '" + meshName + "' uploaded (" +
                     std::to_string(allVertices->size()) + " verts, " +
                     std::to_string(allIndices->size() / 3) + " triangles)");
    }
}

}  // namespace sdl3cpp::services::impl
