#include "services/interfaces/workflow/graphics/workflow_graphics_buffer_upload_step.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>
#include <utility>
#include <cstring>
#include <cstdint>
#include <stdexcept>
#include <vector>

using json = nlohmann::json;

namespace sdl3cpp::services::impl {

WorkflowGraphicsBufferUploadStep::WorkflowGraphicsBufferUploadStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGraphicsBufferUploadStep::GetPluginId() const {
    return "graphics.buffer.upload";
}

void WorkflowGraphicsBufferUploadStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    if (logger_) {
        logger_->Trace("WorkflowGraphicsBufferUploadStep", "Execute", "", "Entry");
    }

    // Read configurable context keys from parameters
    std::string vertex_data_key = "vertex_data";
    std::string index_data_key = "index_data";
    std::string vertex_buffer_key = "gpu_vertex_buffer";
    std::string index_buffer_key = "gpu_index_buffer";
    int vertex_stride = 16;

    auto it = step.parameters.find("vertex_data_key");
    if (it != step.parameters.end()) {
        vertex_data_key = it->second.stringValue;
    }

    it = step.parameters.find("index_data_key");
    if (it != step.parameters.end()) {
        index_data_key = it->second.stringValue;
    }

    it = step.parameters.find("vertex_buffer_key");
    if (it != step.parameters.end()) {
        vertex_buffer_key = it->second.stringValue;
    }

    it = step.parameters.find("index_buffer_key");
    if (it != step.parameters.end()) {
        index_buffer_key = it->second.stringValue;
    }

    it = step.parameters.find("vertex_stride");
    if (it != step.parameters.end()) {
        vertex_stride = static_cast<int>(it->second.numberValue);
    }

    try {
        // Get GPU device
        SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
        if (!device) {
            throw std::runtime_error("graphics.buffer.upload: GPU device not found in context");
        }

        // Read vertex data (flat byte array stored as JSON array of ints)
        const auto* vertex_json = context.TryGet<json>(vertex_data_key);
        if (!vertex_json || !vertex_json->is_array() || vertex_json->empty()) {
            throw std::runtime_error("graphics.buffer.upload: '" + vertex_data_key
                                     + "' not found or not a non-empty array in context");
        }

        // Read index data (array of uint16 values stored as JSON array of ints)
        const auto* index_json = context.TryGet<json>(index_data_key);
        if (!index_json || !index_json->is_array() || index_json->empty()) {
            throw std::runtime_error("graphics.buffer.upload: '" + index_data_key
                                     + "' not found or not a non-empty array in context");
        }

        // Convert vertex JSON byte array to raw bytes
        std::vector<uint8_t> vertex_bytes;
        vertex_bytes.reserve(vertex_json->size());
        for (const auto& v : *vertex_json) {
            if (!v.is_number()) {
                throw std::runtime_error("graphics.buffer.upload: vertex data must be array of numbers");
            }
            vertex_bytes.push_back(static_cast<uint8_t>(v.get<int>()));
        }

        // Convert index JSON array to uint16 vector
        std::vector<uint16_t> index_values;
        index_values.reserve(index_json->size());
        for (const auto& idx : *index_json) {
            if (!idx.is_number()) {
                throw std::runtime_error("graphics.buffer.upload: index data must be array of numbers");
            }
            index_values.push_back(static_cast<uint16_t>(idx.get<int>()));
        }

        uint32_t vertex_size = static_cast<uint32_t>(vertex_bytes.size());
        uint32_t index_size = static_cast<uint32_t>(index_values.size() * sizeof(uint16_t));

        // Create GPU vertex buffer
        SDL_GPUBufferCreateInfo vbuf_info = {};
        vbuf_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vbuf_info.size = vertex_size;

        SDL_GPUBuffer* vbuf = SDL_CreateGPUBuffer(device, &vbuf_info);
        if (!vbuf) {
            throw std::runtime_error("graphics.buffer.upload: Failed to create vertex buffer: "
                                     + std::string(SDL_GetError()));
        }

        // Create GPU index buffer
        SDL_GPUBufferCreateInfo ibuf_info = {};
        ibuf_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        ibuf_info.size = index_size;

        SDL_GPUBuffer* ibuf = SDL_CreateGPUBuffer(device, &ibuf_info);
        if (!ibuf) {
            SDL_ReleaseGPUBuffer(device, vbuf);
            throw std::runtime_error("graphics.buffer.upload: Failed to create index buffer: "
                                     + std::string(SDL_GetError()));
        }

        // Upload both buffers in one transfer
        uint32_t transfer_size = vertex_size + index_size;
        SDL_GPUTransferBufferCreateInfo transfer_info = {};
        transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transfer_info.size = transfer_size;

        SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
        if (!transfer) {
            SDL_ReleaseGPUBuffer(device, vbuf);
            SDL_ReleaseGPUBuffer(device, ibuf);
            throw std::runtime_error("graphics.buffer.upload: Failed to create transfer buffer");
        }

        // Map transfer buffer and copy both vertex + index data
        void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
        std::memcpy(mapped, vertex_bytes.data(), vertex_size);
        std::memcpy(static_cast<uint8_t*>(mapped) + vertex_size, index_values.data(), index_size);
        SDL_UnmapGPUTransferBuffer(device, transfer);

        // Submit copy commands to GPU
        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);

        // Upload vertex data
        SDL_GPUTransferBufferLocation v_src = {};
        v_src.transfer_buffer = transfer;
        v_src.offset = 0;

        SDL_GPUBufferRegion v_dst = {};
        v_dst.buffer = vbuf;
        v_dst.offset = 0;
        v_dst.size = vertex_size;

        SDL_UploadToGPUBuffer(copy_pass, &v_src, &v_dst, false);

        // Upload index data
        SDL_GPUTransferBufferLocation i_src = {};
        i_src.transfer_buffer = transfer;
        i_src.offset = vertex_size;

        SDL_GPUBufferRegion i_dst = {};
        i_dst.buffer = ibuf;
        i_dst.offset = 0;
        i_dst.size = index_size;

        SDL_UploadToGPUBuffer(copy_pass, &i_src, &i_dst, false);

        SDL_EndGPUCopyPass(copy_pass);
        SDL_SubmitGPUCommandBuffer(cmd);

        // Release transfer buffer (GPU buffers persist)
        SDL_ReleaseGPUTransferBuffer(device, transfer);

        // Store GPU buffer handles in context
        context.Set<SDL_GPUBuffer*>(vertex_buffer_key, vbuf);
        context.Set<SDL_GPUBuffer*>(index_buffer_key, ibuf);

        // Store metadata as JSON for downstream steps
        int vertex_count = static_cast<int>(vertex_bytes.size()) / vertex_stride;
        int index_count = static_cast<int>(index_values.size());

        json mesh_metadata = {
            {"vertex_buffer_handle", {
                {"valid", true},
                {"vertex_count", vertex_count}
            }},
            {"index_buffer_handle", {
                {"valid", true},
                {"index_count", index_count}
            }},
            {"vertex_layout", {
                {"stride", vertex_stride}
            }}
        };

        context.Set("cube_mesh", mesh_metadata);
        context.Set("geometry_created", true);

        if (logger_) {
            logger_->Info("WorkflowGraphicsBufferUploadStep: Uploaded to GPU ("
                          + std::to_string(vertex_count) + " vertices, "
                          + std::to_string(index_count) + " indices, "
                          + std::to_string(vertex_size) + "+" + std::to_string(index_size)
                          + " bytes, stride=" + std::to_string(vertex_stride) + ")");
        }

    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("WorkflowGraphicsBufferUploadStep::Execute: " + std::string(e.what()));
        }
        context.Set("geometry_created", false);
    }
}

}  // namespace sdl3cpp::services::impl
