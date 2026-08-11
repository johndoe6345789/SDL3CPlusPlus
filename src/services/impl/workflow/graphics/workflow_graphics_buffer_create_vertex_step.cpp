#include "services/interfaces/workflow/graphics/workflow_graphics_buffer_create_vertex_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowGraphicsBufferCreateVertexStep::WorkflowGraphicsBufferCreateVertexStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGraphicsBufferCreateVertexStep::GetPluginId() const {
    return "graphics.buffer.create_vertex";
}

void WorkflowGraphicsBufferCreateVertexStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string verticesKey = resolver.GetRequiredInputKey(step, "vertices");
    const std::string outputHandleKey = resolver.GetRequiredOutputKey(step, "vertex_handle");

    const auto* vertices_json = context.TryGet<nlohmann::json>(verticesKey);
    if (!vertices_json || !vertices_json->is_array()) {
        throw std::runtime_error("graphics.buffer.create_vertex requires vertices input (array of floats)");
    }

    // Convert JSON array to float vector
    std::vector<float> vertex_data;
    for (const auto& v : *vertices_json) {
        if (v.is_number()) {
            vertex_data.push_back(v.get<float>());
        } else {
            throw std::runtime_error("graphics.buffer.create_vertex: all vertices must be numbers");
        }
    }

    if (vertex_data.empty()) {
        throw std::runtime_error("graphics.buffer.create_vertex: vertices array is empty");
    }

    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error("graphics.buffer.create_vertex: GPU device not found in context");
    }

    uint32_t data_size = static_cast<uint32_t>(vertex_data.size() * sizeof(float));

    // Create GPU vertex buffer
    SDL_GPUBufferCreateInfo buf_info = {};
    buf_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    buf_info.size = data_size;

    SDL_GPUBuffer* vbuf = SDL_CreateGPUBuffer(device, &buf_info);
    if (!vbuf) {
        throw std::runtime_error("graphics.buffer.create_vertex: SDL_CreateGPUBuffer failed: " +
                                 std::string(SDL_GetError()));
    }

    // Create transfer buffer and upload data
    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = data_size;

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    if (!transfer) {
        SDL_ReleaseGPUBuffer(device, vbuf);
        throw std::runtime_error("graphics.buffer.create_vertex: Failed to create transfer buffer");
    }

    // Map, copy, unmap
    void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    memcpy(mapped, vertex_data.data(), data_size);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    // Upload via copy pass
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation src = {};
    src.transfer_buffer = transfer;
    src.offset = 0;

    SDL_GPUBufferRegion dst = {};
    dst.buffer = vbuf;
    dst.offset = 0;
    dst.size = data_size;

    SDL_UploadToGPUBuffer(copy_pass, &src, &dst, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cmd);

    // Release transfer buffer (GPU buffer persists)
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    if (logger_) {
        logger_->Trace("WorkflowGraphicsBufferCreateVertexStep", "Execute",
                       "vertex_count=" + std::to_string(vertex_data.size() / 3),
                       "Vertex buffer created successfully");
    }

    // Store buffer pointer in context
    context.Set<SDL_GPUBuffer*>("gpu_vertex_buffer", vbuf);

    nlohmann::json buffer_data = {
        {"valid", true},
        {"vertex_count", vertex_data.size() / 3},
        {"size_bytes", data_size}
    };
    context.Set(outputHandleKey, buffer_data);
}

}  // namespace sdl3cpp::services::impl
