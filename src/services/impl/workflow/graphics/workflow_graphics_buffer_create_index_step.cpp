#include "services/interfaces/workflow/graphics/workflow_graphics_buffer_create_index_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <cstring>

namespace sdl3cpp::services::impl {

WorkflowGraphicsBufferCreateIndexStep::WorkflowGraphicsBufferCreateIndexStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGraphicsBufferCreateIndexStep::GetPluginId() const {
    return "graphics.buffer.create_index";
}

void WorkflowGraphicsBufferCreateIndexStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string indicesKey = resolver.GetRequiredInputKey(step, "indices");
    const std::string outputHandleKey = resolver.GetRequiredOutputKey(step, "index_handle");

    const auto* indices_json = context.TryGet<nlohmann::json>(indicesKey);
    if (!indices_json || !indices_json->is_array()) {
        throw std::runtime_error("graphics.buffer.create_index requires indices input (array of integers)");
    }

    // Convert JSON array to uint16 vector
    std::vector<uint16_t> index_data;
    for (const auto& idx : *indices_json) {
        if (idx.is_number()) {
            index_data.push_back(static_cast<uint16_t>(idx.get<int>()));
        } else {
            throw std::runtime_error("graphics.buffer.create_index: all indices must be numbers");
        }
    }

    if (index_data.empty()) {
        throw std::runtime_error("graphics.buffer.create_index: indices array is empty");
    }

    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error("graphics.buffer.create_index: GPU device not found in context");
    }

    uint32_t data_size = static_cast<uint32_t>(index_data.size() * sizeof(uint16_t));

    // Create GPU index buffer
    SDL_GPUBufferCreateInfo buf_info = {};
    buf_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    buf_info.size = data_size;

    SDL_GPUBuffer* ibuf = SDL_CreateGPUBuffer(device, &buf_info);
    if (!ibuf) {
        throw std::runtime_error("graphics.buffer.create_index: SDL_CreateGPUBuffer failed: " +
                                 std::string(SDL_GetError()));
    }

    // Upload via transfer buffer
    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = data_size;

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    if (!transfer) {
        SDL_ReleaseGPUBuffer(device, ibuf);
        throw std::runtime_error("graphics.buffer.create_index: Failed to create transfer buffer");
    }

    void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    memcpy(mapped, index_data.data(), data_size);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation src = {};
    src.transfer_buffer = transfer;
    src.offset = 0;

    SDL_GPUBufferRegion dst = {};
    dst.buffer = ibuf;
    dst.offset = 0;
    dst.size = data_size;

    SDL_UploadToGPUBuffer(copy_pass, &src, &dst, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cmd);

    SDL_ReleaseGPUTransferBuffer(device, transfer);

    if (logger_) {
        logger_->Trace("WorkflowGraphicsBufferCreateIndexStep", "Execute",
                       "index_count=" + std::to_string(index_data.size()),
                       "Index buffer created successfully");
    }

    // Store buffer pointer in context
    context.Set<SDL_GPUBuffer*>("gpu_index_buffer", ibuf);

    nlohmann::json buffer_data = {
        {"valid", true},
        {"index_count", index_data.size()},
        {"size_bytes", data_size}
    };
    context.Set(outputHandleKey, buffer_data);
}

}  // namespace sdl3cpp::services::impl
