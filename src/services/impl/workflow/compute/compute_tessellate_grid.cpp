#include "services/interfaces/workflow/compute/compute_tessellate_grid.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <nlohmann/json.hpp>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

float NumberParameter(const WorkflowStepParameterResolver& params,
                      const WorkflowStepDefinition& step, const char* key,
                      float fallback) {
    const auto* p       = params.FindParameter(step, key);
    const bool isNumber = p && p->type == WorkflowParameterValue::Type::Number;
    return isNumber ? static_cast<float>(p->numberValue) : fallback;
}

std::string StringParameter(const WorkflowStepParameterResolver& params,
                            const WorkflowStepDefinition& step, const char* key,
                            const std::string& fallback) {
    const auto* p       = params.FindParameter(step, key);
    const bool isString = p && p->type == WorkflowParameterValue::Type::String;
    return isString ? p->stringValue : fallback;
}

/// A displacement-mapped grid's compute uniform, matching the shader's layout.
struct TessellationUniform {
    float width;
    float depth;
    float displacementStrength;
    float uvScaleX;
    float uvScaleY;
    uint32_t subdivisions;
    uint32_t _pad0;
    uint32_t _pad1;
};

std::vector<uint16_t> GenerateGridIndices(int subdivisions,
                                          uint32_t vertsPerSide) {
    std::vector<uint16_t> indices;
    indices.reserve(static_cast<size_t>(subdivisions) * subdivisions * 6);
    for (int iy = 0; iy < subdivisions; ++iy) {
        for (int ix = 0; ix < subdivisions; ++ix) {
            const uint16_t tl = static_cast<uint16_t>(iy * vertsPerSide + ix);
            const uint16_t tr = tl + 1;
            const uint16_t bl =
                static_cast<uint16_t>((iy + 1) * vertsPerSide + ix);
            const uint16_t br = bl + 1;
            indices.push_back(tl);
            indices.push_back(bl);
            indices.push_back(tr);
            indices.push_back(tr);
            indices.push_back(bl);
            indices.push_back(br);
        }
    }
    return indices;
}

}  // namespace

TessellationGridParams ReadTessellationGridParams(
    const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver params;
    TessellationGridParams p;
    p.width                = NumberParameter(params, step, "width", p.width);
    p.depth                = NumberParameter(params, step, "depth", p.depth);
    p.subdivisions         = static_cast<int>(NumberParameter(
        params, step, "subdivisions", static_cast<float>(p.subdivisions)));
    p.displacementStrength = NumberParameter(
        params, step, "displacement_strength", p.displacementStrength);
    p.uvScaleX = NumberParameter(params, step, "uv_scale_x", p.uvScaleX);
    p.uvScaleY = NumberParameter(params, step, "uv_scale_y", p.uvScaleY);
    p.name     = StringParameter(params, step, "name", p.name);
    return p;
}

TessellationGridBuffers CreateAndUploadTessellationGrid(SDL_GPUDevice* device,
                                                        int subdivisions) {
    TessellationGridBuffers buffers;
    const uint32_t vertsPerSide = static_cast<uint32_t>(subdivisions + 1);
    buffers.vertexCount         = vertsPerSide * vertsPerSide;
    buffers.indexCount = static_cast<uint32_t>(subdivisions * subdivisions * 6);

    SDL_GPUBufferCreateInfo vbufInfo = {};
    vbufInfo.usage =
        SDL_GPU_BUFFERUSAGE_VERTEX | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
    vbufInfo.size        = buffers.vertexCount * buffers.vertexStride;
    buffers.vertexBuffer = SDL_CreateGPUBuffer(device, &vbufInfo);
    if (!buffers.vertexBuffer) {
        throw std::runtime_error(
            "compute.tessellate: Failed to create vertex buffer");
    }

    const uint32_t indexSize         = buffers.indexCount * sizeof(uint16_t);
    SDL_GPUBufferCreateInfo ibufInfo = {};
    ibufInfo.usage                   = SDL_GPU_BUFFERUSAGE_INDEX;
    ibufInfo.size                    = indexSize;
    buffers.indexBuffer              = SDL_CreateGPUBuffer(device, &ibufInfo);
    if (!buffers.indexBuffer) {
        SDL_ReleaseGPUBuffer(device, buffers.vertexBuffer);
        throw std::runtime_error(
            "compute.tessellate: Failed to create index buffer");
    }

    SDL_GPUTransferBufferCreateInfo tbufInfo = {};
    tbufInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbufInfo.size  = indexSize;
    SDL_GPUTransferBuffer* transfer =
        SDL_CreateGPUTransferBuffer(device, &tbufInfo);
    if (!transfer) {
        SDL_ReleaseGPUBuffer(device, buffers.vertexBuffer);
        SDL_ReleaseGPUBuffer(device, buffers.indexBuffer);
        throw std::runtime_error(
            "compute.tessellate: Failed to create transfer buffer");
    }

    const std::vector<uint16_t> indices =
        GenerateGridIndices(subdivisions, vertsPerSide);
    if (void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false)) {
        std::memcpy(mapped, indices.data(), indexSize);
        SDL_UnmapGPUTransferBuffer(device, transfer);
    }

    SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(device);
    if (SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(uploadCmd)) {
        SDL_GPUTransferBufferLocation src = {transfer, 0};
        SDL_GPUBufferRegion dst           = {buffers.indexBuffer, 0, indexSize};
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);
    }
    SDL_SubmitGPUCommandBuffer(uploadCmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    return buffers;
}

void DispatchTessellationCompute(SDL_GPUDevice* device,
                                 SDL_GPUComputePipeline* pipeline,
                                 SDL_GPUTexture* displacementTexture,
                                 SDL_GPUSampler* displacementSampler,
                                 const TessellationGridParams& params,
                                 const TessellationGridBuffers& buffers) {
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);

    SDL_GPUStorageBufferReadWriteBinding rwBinding = {};
    rwBinding.buffer                               = buffers.vertexBuffer;
    rwBinding.cycle                                = true;

    SDL_GPUComputePass* pass =
        SDL_BeginGPUComputePass(cmd, nullptr, 0, &rwBinding, 1);
    SDL_BindGPUComputePipeline(pass, pipeline);

    SDL_GPUTextureSamplerBinding texBinding = {displacementTexture,
                                               displacementSampler};
    SDL_BindGPUComputeSamplers(pass, 0, &texBinding, 1);

    TessellationUniform uniform  = {};
    uniform.width                = params.width;
    uniform.depth                = params.depth;
    uniform.displacementStrength = params.displacementStrength;
    uniform.uvScaleX             = params.uvScaleX;
    uniform.uvScaleY             = params.uvScaleY;
    uniform.subdivisions         = static_cast<uint32_t>(params.subdivisions);
    SDL_PushGPUComputeUniformData(cmd, 0, &uniform, sizeof(uniform));

    const uint32_t vertsPerSide =
        static_cast<uint32_t>(params.subdivisions + 1);
    const uint32_t groupsX = (vertsPerSide + 7) / 8;
    const uint32_t groupsY = (vertsPerSide + 7) / 8;
    SDL_DispatchGPUCompute(pass, groupsX, groupsY, 1);

    SDL_EndGPUComputePass(pass);
    SDL_SubmitGPUCommandBuffer(cmd);
}

void PublishTessellationGrid(WorkflowContext& context,
                             const TessellationGridParams& params,
                             const TessellationGridBuffers& buffers) {
    context.Set<SDL_GPUBuffer*>("plane_" + params.name + "_vb",
                                buffers.vertexBuffer);
    context.Set<SDL_GPUBuffer*>("plane_" + params.name + "_ib",
                                buffers.indexBuffer);

    context.Set(
        "plane_" + params.name,
        nlohmann::json{{"vertex_count", buffers.vertexCount},
                       {"index_count", buffers.indexCount},
                       {"stride", buffers.vertexStride},
                       {"width", params.width},
                       {"depth", params.depth},
                       {"subdivisions", params.subdivisions},
                       {"displacement_strength", params.displacementStrength},
                       {"compute_tessellated", true}});
}

}  // namespace sdl3cpp::services::impl
