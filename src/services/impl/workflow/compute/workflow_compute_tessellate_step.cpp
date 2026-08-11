#include "services/interfaces/workflow/compute/workflow_compute_tessellate_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowComputeTessellateStep::WorkflowComputeTessellateStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowComputeTessellateStep::GetPluginId() const {
    return "compute.tessellate";
}

void WorkflowComputeTessellateStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepParameterResolver params;
    WorkflowStepIoResolver resolver;

    auto getNum = [&](const char* pname, float def) -> float {
        const auto* p = params.FindParameter(step, pname);
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
    const float depth = getNum("depth", 5.0f);
    const int subdivisions = getInt("subdivisions", 64);
    const float disp_strength = getNum("displacement_strength", 0.1f);
    const float uv_scale_x = getNum("uv_scale_x", 1.0f);
    const float uv_scale_y = getNum("uv_scale_y", 1.0f);
    const std::string name = getStr("name", "tessellated");

    // Get displacement texture name from inputs
    const std::string dispTexKey = resolver.GetRequiredInputKey(step, "displacement_texture");
    const std::string shaderPathKey = resolver.GetRequiredInputKey(step, "compute_shader_path");

    // Get shader path from context
    const auto* shader_path = context.TryGet<std::string>(shaderPathKey);
    if (!shader_path) {
        throw std::runtime_error("compute.tessellate: compute_shader_path not found in context key '" + shaderPathKey + "'");
    }

    // Resolve ~ in path
    std::string resolved_shader = *shader_path;
    if (!resolved_shader.empty() && resolved_shader[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home) resolved_shader = std::string(home) + resolved_shader.substr(1);
    }

    // Get displacement texture + sampler from context
    auto* disp_texture = context.Get<SDL_GPUTexture*>(dispTexKey + "_gpu", nullptr);
    auto* disp_sampler = context.Get<SDL_GPUSampler*>(dispTexKey + "_sampler", nullptr);
    if (!disp_texture || !disp_sampler) {
        throw std::runtime_error("compute.tessellate: Displacement texture '" + dispTexKey + "' not found in context");
    }

    // Get GPU device
    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error("compute.tessellate: GPU device not found in context");
    }

    // Load compute shader binary
    std::ifstream file(resolved_shader, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("compute.tessellate: Failed to open shader: " + resolved_shader);
    }
    std::streamsize shader_size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> shader_data(shader_size);
    file.read(reinterpret_cast<char*>(shader_data.data()), shader_size);

    // Detect shader format
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_SPIRV;
    const char* driver = SDL_GetGPUDeviceDriver(device);
    if (driver && std::string(driver) == "metal") {
        format = SDL_GPU_SHADERFORMAT_MSL;
    }
    const char* entrypoint = (format == SDL_GPU_SHADERFORMAT_MSL) ? "main0" : "main";

    // Create compute pipeline
    SDL_GPUComputePipelineCreateInfo pipeline_info = {};
    pipeline_info.code = shader_data.data();
    pipeline_info.code_size = shader_data.size();
    pipeline_info.entrypoint = entrypoint;
    pipeline_info.format = format;
    pipeline_info.num_samplers = 1;
    pipeline_info.num_readwrite_storage_buffers = 1;
    pipeline_info.num_uniform_buffers = 1;
    pipeline_info.threadcount_x = 8;
    pipeline_info.threadcount_y = 8;
    pipeline_info.threadcount_z = 1;

    SDL_GPUComputePipeline* pipeline = SDL_CreateGPUComputePipeline(device, &pipeline_info);
    if (!pipeline) {
        throw std::runtime_error("compute.tessellate: Failed to create compute pipeline: " +
                                 std::string(SDL_GetError()));
    }

    const uint32_t verts_per_side = static_cast<uint32_t>(subdivisions + 1);
    const uint32_t vertex_count = verts_per_side * verts_per_side;
    const uint32_t index_count = static_cast<uint32_t>(subdivisions * subdivisions * 6);
    const uint32_t vertex_stride = 20;  // float3 + float2 = 20 bytes
    const uint32_t vertex_size = vertex_count * vertex_stride;
    const uint32_t index_size = index_count * sizeof(uint16_t);

    // Create output vertex buffer (dual usage: vertex + compute write)
    SDL_GPUBufferCreateInfo vbuf_info = {};
    vbuf_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
    vbuf_info.size = vertex_size;

    SDL_GPUBuffer* vertex_buffer = SDL_CreateGPUBuffer(device, &vbuf_info);
    if (!vertex_buffer) {
        SDL_ReleaseGPUComputePipeline(device, pipeline);
        throw std::runtime_error("compute.tessellate: Failed to create vertex buffer");
    }

    // Generate index buffer on CPU (grid pattern)
    std::vector<uint16_t> indices;
    indices.reserve(index_count);
    for (int iy = 0; iy < subdivisions; ++iy) {
        for (int ix = 0; ix < subdivisions; ++ix) {
            uint16_t tl = static_cast<uint16_t>(iy * verts_per_side + ix);
            uint16_t tr = tl + 1;
            uint16_t bl = static_cast<uint16_t>((iy + 1) * verts_per_side + ix);
            uint16_t br = bl + 1;
            indices.push_back(tl);
            indices.push_back(bl);
            indices.push_back(tr);
            indices.push_back(tr);
            indices.push_back(bl);
            indices.push_back(br);
        }
    }

    // Create and upload index buffer
    SDL_GPUBufferCreateInfo ibuf_info = {};
    ibuf_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    ibuf_info.size = index_size;

    SDL_GPUBuffer* index_buffer = SDL_CreateGPUBuffer(device, &ibuf_info);
    if (!index_buffer) {
        SDL_ReleaseGPUBuffer(device, vertex_buffer);
        SDL_ReleaseGPUComputePipeline(device, pipeline);
        throw std::runtime_error("compute.tessellate: Failed to create index buffer");
    }

    // Upload indices via transfer buffer
    SDL_GPUTransferBufferCreateInfo tbuf_info = {};
    tbuf_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbuf_info.size = index_size;

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &tbuf_info);
    if (!transfer) {
        SDL_ReleaseGPUBuffer(device, vertex_buffer);
        SDL_ReleaseGPUBuffer(device, index_buffer);
        SDL_ReleaseGPUComputePipeline(device, pipeline);
        throw std::runtime_error("compute.tessellate: Failed to create transfer buffer");
    }

    auto* mapped = static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(device, transfer, false));
    std::memcpy(mapped, indices.data(), index_size);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    // Upload indices
    SDL_GPUCommandBuffer* upload_cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_cmd);

    SDL_GPUTransferBufferLocation src = {};
    src.transfer_buffer = transfer;
    src.offset = 0;

    SDL_GPUBufferRegion dst = {};
    dst.buffer = index_buffer;
    dst.offset = 0;
    dst.size = index_size;

    SDL_UploadToGPUBuffer(copy_pass, &src, &dst, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    // Run compute shader to fill vertex buffer
    SDL_GPUCommandBuffer* compute_cmd = SDL_AcquireGPUCommandBuffer(device);

    SDL_GPUStorageBufferReadWriteBinding rw_binding = {};
    rw_binding.buffer = vertex_buffer;
    rw_binding.cycle = true;

    SDL_GPUComputePass* compute_pass = SDL_BeginGPUComputePass(
        compute_cmd, nullptr, 0, &rw_binding, 1);

    SDL_BindGPUComputePipeline(compute_pass, pipeline);

    // Bind displacement texture + sampler
    SDL_GPUTextureSamplerBinding tex_binding = {};
    tex_binding.texture = disp_texture;
    tex_binding.sampler = disp_sampler;
    SDL_BindGPUComputeSamplers(compute_pass, 0, &tex_binding, 1);

    // Push tessellation parameters as uniform
    struct TessParams {
        float width;
        float depth;
        float displacement_strength;
        float uv_scale_x;
        float uv_scale_y;
        uint32_t subdivisions;
        uint32_t _pad0;
        uint32_t _pad1;
    };

    TessParams tess_params = {};
    tess_params.width = width;
    tess_params.depth = depth;
    tess_params.displacement_strength = disp_strength;
    tess_params.uv_scale_x = uv_scale_x;
    tess_params.uv_scale_y = uv_scale_y;
    tess_params.subdivisions = static_cast<uint32_t>(subdivisions);

    SDL_PushGPUComputeUniformData(compute_cmd, 0, &tess_params, sizeof(tess_params));

    // Dispatch workgroups
    uint32_t groups_x = (verts_per_side + 7) / 8;
    uint32_t groups_y = (verts_per_side + 7) / 8;
    SDL_DispatchGPUCompute(compute_pass, groups_x, groups_y, 1);

    SDL_EndGPUComputePass(compute_pass);
    SDL_SubmitGPUCommandBuffer(compute_cmd);

    // Release compute pipeline (no longer needed after dispatch)
    SDL_ReleaseGPUComputePipeline(device, pipeline);

    // Store buffers in context (same convention as geometry.create_plane)
    context.Set<SDL_GPUBuffer*>("plane_" + name + "_vb", vertex_buffer);
    context.Set<SDL_GPUBuffer*>("plane_" + name + "_ib", index_buffer);

    nlohmann::json meta = {
        {"vertex_count", vertex_count},
        {"index_count", index_count},
        {"stride", vertex_stride},
        {"width", width},
        {"depth", depth},
        {"subdivisions", subdivisions},
        {"displacement_strength", disp_strength},
        {"compute_tessellated", true}
    };
    context.Set("plane_" + name, meta);

    if (logger_) {
        logger_->Info("compute.tessellate: '" + name + "' created (" +
                     std::to_string(vertex_count) + " verts, " +
                     std::to_string(index_count) + " indices, " +
                     std::to_string(subdivisions) + "x" + std::to_string(subdivisions) +
                     " subdivisions, disp=" + std::to_string(disp_strength) + ")");
    }
}

}  // namespace sdl3cpp::services::impl
