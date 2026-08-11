#include "services/interfaces/workflow/rendering/workflow_geometry_create_flashlight_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>
#include <cmath>
#include <cstring>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowGeometryCreateFlashlightStep::WorkflowGeometryCreateFlashlightStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGeometryCreateFlashlightStep::GetPluginId() const {
    return "geometry.create_flashlight";
}

void WorkflowGeometryCreateFlashlightStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepParameterResolver params;

    auto getStr = [&](const char* pname, const std::string& def) -> std::string {
        const auto* p = params.FindParameter(step, pname);
        return (p && p->type == WorkflowParameterValue::Type::String) ? p->stringValue : def;
    };
    auto getNum = [&](const char* pname, float def) -> float {
        const auto* p = params.FindParameter(step, pname);
        return (p && p->type == WorkflowParameterValue::Type::Number) ? static_cast<float>(p->numberValue) : def;
    };

    const std::string name = getStr("name", "flashlight");
    const int segments = static_cast<int>(getNum("segments", 12));
    const float body_radius = getNum("body_radius", 0.025f);
    const float body_length = getNum("body_length", 0.25f);
    const float head_radius = getNum("head_radius", 0.04f);
    const float head_length = getNum("head_length", 0.08f);
    const float lens_radius = getNum("lens_radius", 0.035f);

    // Vertex format: float3 pos + float2 uv = 20 bytes (matches plane/textured pipeline)
    struct PosUvVertex {
        float x, y, z;
        float u, v;
    };

    std::vector<PosUvVertex> vertices;
    std::vector<uint16_t> indices;

    const float PI = 3.14159265358979f;

    // Helper: add a cylinder section
    auto addCylinder = [&](float r1, float r2, float y_start, float y_end, float uv_start, float uv_end) {
        uint16_t base = static_cast<uint16_t>(vertices.size());

        for (int i = 0; i <= segments; ++i) {
            float angle = (static_cast<float>(i) / segments) * 2.0f * PI;
            float cos_a = std::cos(angle);
            float sin_a = std::sin(angle);
            float u = static_cast<float>(i) / segments;

            // Bottom ring
            vertices.push_back({cos_a * r1, y_start, sin_a * r1, u, uv_start});
            // Top ring
            vertices.push_back({cos_a * r2, y_end, sin_a * r2, u, uv_end});
        }

        for (int i = 0; i < segments; ++i) {
            uint16_t b = base + static_cast<uint16_t>(i * 2);
            indices.push_back(b);
            indices.push_back(b + 1);
            indices.push_back(b + 2);
            indices.push_back(b + 2);
            indices.push_back(b + 1);
            indices.push_back(b + 3);
        }
    };

    // Helper: add a disc cap
    auto addCap = [&](float radius, float y, float uv_v, bool flip) {
        uint16_t center = static_cast<uint16_t>(vertices.size());
        vertices.push_back({0.0f, y, 0.0f, 0.5f, uv_v});

        for (int i = 0; i <= segments; ++i) {
            float angle = (static_cast<float>(i) / segments) * 2.0f * PI;
            vertices.push_back({std::cos(angle) * radius, y, std::sin(angle) * radius,
                                0.5f + 0.5f * std::cos(angle), uv_v});
        }

        for (int i = 0; i < segments; ++i) {
            uint16_t a = center + 1 + static_cast<uint16_t>(i);
            uint16_t b = a + 1;
            if (flip) {
                indices.push_back(center); indices.push_back(b); indices.push_back(a);
            } else {
                indices.push_back(center); indices.push_back(a); indices.push_back(b);
            }
        }
    };

    // Build flashlight geometry (Y-axis is the barrel direction)
    // Body: long cylinder (handle/grip)
    addCap(body_radius, 0.0f, 0.0f, true);  // bottom cap
    addCylinder(body_radius, body_radius, 0.0f, body_length, 0.0f, 0.6f);

    // Head: slightly wider cylinder (where the bulb sits)
    addCylinder(body_radius, head_radius, body_length, body_length + 0.02f, 0.6f, 0.7f);
    addCylinder(head_radius, head_radius, body_length + 0.02f, body_length + head_length, 0.7f, 0.9f);

    // Lens: flat disc at the front (the light-emitting surface)
    addCap(lens_radius, body_length + head_length, 1.0f, false);

    const uint32_t vertex_count = static_cast<uint32_t>(vertices.size());
    const uint32_t index_count = static_cast<uint32_t>(indices.size());
    const uint32_t vertex_size = vertex_count * sizeof(PosUvVertex);
    const uint32_t index_size = index_count * sizeof(uint16_t);

    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) throw std::runtime_error("geometry.create_flashlight: GPU device not found");

    SDL_GPUBufferCreateInfo vbuf_info = {};
    vbuf_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vbuf_info.size = vertex_size;
    SDL_GPUBuffer* vertex_buffer = SDL_CreateGPUBuffer(device, &vbuf_info);

    SDL_GPUBufferCreateInfo ibuf_info = {};
    ibuf_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    ibuf_info.size = index_size;
    SDL_GPUBuffer* index_buffer = SDL_CreateGPUBuffer(device, &ibuf_info);

    SDL_GPUTransferBufferCreateInfo tbuf_info = {};
    tbuf_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbuf_info.size = vertex_size + index_size;
    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &tbuf_info);

    auto* mapped = static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(device, transfer, false));
    std::memcpy(mapped, vertices.data(), vertex_size);
    std::memcpy(mapped + vertex_size, indices.data(), index_size);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation src_vert = {}; src_vert.transfer_buffer = transfer;
    SDL_GPUBufferRegion dst_vert = {}; dst_vert.buffer = vertex_buffer; dst_vert.size = vertex_size;
    SDL_UploadToGPUBuffer(copy_pass, &src_vert, &dst_vert, false);

    SDL_GPUTransferBufferLocation src_idx = {}; src_idx.transfer_buffer = transfer; src_idx.offset = vertex_size;
    SDL_GPUBufferRegion dst_idx = {}; dst_idx.buffer = index_buffer; dst_idx.size = index_size;
    SDL_UploadToGPUBuffer(copy_pass, &src_idx, &dst_idx, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    context.Set<SDL_GPUBuffer*>("plane_" + name + "_vb", vertex_buffer);
    context.Set<SDL_GPUBuffer*>("plane_" + name + "_ib", index_buffer);
    context.Set("plane_" + name, nlohmann::json{
        {"vertex_count", vertex_count}, {"index_count", index_count}, {"stride", 20}
    });

    if (logger_) {
        logger_->Info("geometry.create_flashlight: '" + name + "' created (" +
                     std::to_string(vertex_count) + " verts, " +
                     std::to_string(index_count) + " indices)");
    }
}

}  // namespace sdl3cpp::services::impl
