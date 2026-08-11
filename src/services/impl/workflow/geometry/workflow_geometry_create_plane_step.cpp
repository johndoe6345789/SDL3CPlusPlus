#include "services/interfaces/workflow/geometry/workflow_geometry_create_plane_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowGeometryCreatePlaneStep::WorkflowGeometryCreatePlaneStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGeometryCreatePlaneStep::GetPluginId() const {
    return "geometry.create_plane";
}

void WorkflowGeometryCreatePlaneStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepParameterResolver params;

    auto getNum = [&](const char* name, float def) -> float {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number) ? static_cast<float>(p->numberValue) : def;
    };
    auto getInt = [&](const char* pname, int def) -> int {
        const auto* p = params.FindParameter(step, pname);
        return (p && p->type == WorkflowParameterValue::Type::Number) ? static_cast<int>(p->numberValue) : def;
    };
    auto getStr = [&](const char* pname, const std::string& def) -> std::string {
        const auto* p = params.FindParameter(step, pname);
        return (p && p->type == WorkflowParameterValue::Type::String) ? p->stringValue : def;
    };

    const float width = getNum("width", 10.0f);
    const float depth = getNum("depth", 10.0f);
    const float uv_scale_x = getNum("uv_scale_x", 1.0f);
    const float uv_scale_y = getNum("uv_scale_y", 1.0f);
    const int subdiv_x = getInt("subdivisions_x", 1);
    const int subdiv_y = getInt("subdivisions_y", 1);
    const std::string name = getStr("name", "plane");

    // Vertex format: float3 position + float2 uv = 20 bytes
    struct PosUvVertex {
        float x, y, z;
        float u, v;
    };

    const float hw = width * 0.5f;
    const float hd = depth * 0.5f;
    const int verts_x = subdiv_x + 1;
    const int verts_y = subdiv_y + 1;

    std::vector<PosUvVertex> vertices;
    vertices.reserve(verts_x * verts_y);

    for (int iy = 0; iy < verts_y; ++iy) {
        float fy = static_cast<float>(iy) / static_cast<float>(subdiv_y);
        for (int ix = 0; ix < verts_x; ++ix) {
            float fx = static_cast<float>(ix) / static_cast<float>(subdiv_x);
            PosUvVertex v;
            v.x = -hw + fx * width;
            v.y = 0.0f;
            v.z = -hd + fy * depth;
            v.u = fx * uv_scale_x;
            v.v = fy * uv_scale_y;
            vertices.push_back(v);
        }
    }

    std::vector<uint16_t> indices;
    indices.reserve(subdiv_x * subdiv_y * 6);

    for (int iy = 0; iy < subdiv_y; ++iy) {
        for (int ix = 0; ix < subdiv_x; ++ix) {
            uint16_t tl = static_cast<uint16_t>(iy * verts_x + ix);
            uint16_t tr = tl + 1;
            uint16_t bl = static_cast<uint16_t>((iy + 1) * verts_x + ix);
            uint16_t br = bl + 1;

            indices.push_back(tl);
            indices.push_back(bl);
            indices.push_back(tr);
            indices.push_back(tr);
            indices.push_back(bl);
            indices.push_back(br);
        }
    }

    const uint32_t vertex_count = static_cast<uint32_t>(vertices.size());
    const uint32_t index_count = static_cast<uint32_t>(indices.size());
    const uint32_t vertex_size = vertex_count * sizeof(PosUvVertex);
    const uint32_t index_size = index_count * sizeof(uint16_t);

    // Get GPU device
    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error("geometry.create_plane: GPU device not found in context");
    }

    // Create GPU buffers
    SDL_GPUBufferCreateInfo vbuf_info = {};
    vbuf_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vbuf_info.size = vertex_size;

    SDL_GPUBuffer* vertex_buffer = SDL_CreateGPUBuffer(device, &vbuf_info);
    if (!vertex_buffer) {
        throw std::runtime_error("geometry.create_plane: Failed to create vertex buffer");
    }

    SDL_GPUBufferCreateInfo ibuf_info = {};
    ibuf_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    ibuf_info.size = index_size;

    SDL_GPUBuffer* index_buffer = SDL_CreateGPUBuffer(device, &ibuf_info);
    if (!index_buffer) {
        SDL_ReleaseGPUBuffer(device, vertex_buffer);
        throw std::runtime_error("geometry.create_plane: Failed to create index buffer");
    }

    // Upload via single transfer buffer
    SDL_GPUTransferBufferCreateInfo tbuf_info = {};
    tbuf_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbuf_info.size = vertex_size + index_size;

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &tbuf_info);
    if (!transfer) {
        SDL_ReleaseGPUBuffer(device, vertex_buffer);
        SDL_ReleaseGPUBuffer(device, index_buffer);
        throw std::runtime_error("geometry.create_plane: Failed to create transfer buffer");
    }

    auto* mapped = static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(device, transfer, false));
    std::memcpy(mapped, vertices.data(), vertex_size);
    std::memcpy(mapped + vertex_size, indices.data(), index_size);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation src_vert = {};
    src_vert.transfer_buffer = transfer;
    src_vert.offset = 0;

    SDL_GPUBufferRegion dst_vert = {};
    dst_vert.buffer = vertex_buffer;
    dst_vert.offset = 0;
    dst_vert.size = vertex_size;

    SDL_UploadToGPUBuffer(copy_pass, &src_vert, &dst_vert, false);

    SDL_GPUTransferBufferLocation src_idx = {};
    src_idx.transfer_buffer = transfer;
    src_idx.offset = vertex_size;

    SDL_GPUBufferRegion dst_idx = {};
    dst_idx.buffer = index_buffer;
    dst_idx.offset = 0;
    dst_idx.size = index_size;

    SDL_UploadToGPUBuffer(copy_pass, &src_idx, &dst_idx, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    // Store in context keyed by name
    context.Set<SDL_GPUBuffer*>("plane_" + name + "_vb", vertex_buffer);
    context.Set<SDL_GPUBuffer*>("plane_" + name + "_ib", index_buffer);

    nlohmann::json meta = {
        {"vertex_count", vertex_count},
        {"index_count", index_count},
        {"stride", 20},
        {"width", width},
        {"depth", depth},
        {"subdivisions_x", subdiv_x},
        {"subdivisions_y", subdiv_y}
    };
    context.Set("plane_" + name, meta);

    if (logger_) {
        logger_->Info("geometry.create_plane: '" + name + "' created (" +
                     std::to_string(vertex_count) + " verts, " +
                     std::to_string(index_count) + " indices, " +
                     std::to_string(subdiv_x) + "x" + std::to_string(subdiv_y) + " subdivisions)");
    }
}

}  // namespace sdl3cpp::services::impl
