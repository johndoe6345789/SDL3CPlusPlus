#include "services/interfaces/workflow/compute/compute_tessellate_grid.hpp"

namespace sdl3cpp::services::impl {
namespace {

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

}  // namespace

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

}  // namespace sdl3cpp::services::impl
