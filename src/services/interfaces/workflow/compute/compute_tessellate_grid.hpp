#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <SDL3/SDL_gpu.h>
#include <cstdint>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Shared parameters of `compute.tessellate` and its split successor,
 *        `compute.pipeline.create` + `compute.tessellate.dispatch`.
 *
 * Both build the same displacement-mapped grid via the same compute shader
 * contract; only how the compute pipeline itself is obtained differs.
 */
struct TessellationGridParams {
    float width                = 10.0f;
    float depth                = 5.0f;
    int subdivisions           = 64;
    float displacementStrength = 0.1f;
    float uvScaleX             = 1.0f;
    float uvScaleY             = 1.0f;
    std::string name           = "tessellated";
};

TessellationGridParams ReadTessellationGridParams(
    const WorkflowStepDefinition& step);

/// The vertex/index buffers backing one tessellated grid.
struct TessellationGridBuffers {
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer  = nullptr;
    uint32_t vertexCount        = 0;
    uint32_t indexCount         = 0;
    uint32_t vertexStride       = 20;  // float3 position + float2 uv
};

/**
 * @brief Allocates the grid's vertex/index buffers and uploads the indices.
 *
 * The vertex buffer is left empty — DispatchTessellationCompute() fills it.
 * The index buffer is a plain triangle grid, generated on the CPU since it
 * only depends on `subdivisions`, not the displacement texture.
 *
 * @throws std::runtime_error if any GPU resource fails to create.
 */
TessellationGridBuffers CreateAndUploadTessellationGrid(SDL_GPUDevice* device,
                                                        int subdivisions);

/**
 * @brief Runs the compute shader that fills a grid's vertex buffer.
 *
 * Binds `displacementTexture`/`displacementSampler` at compute sampler slot
 * 0, pushes `params` as the compute uniform, and dispatches one thread group
 * per 8x8 block of vertices (matching the shader's declared threadcount).
 */
void DispatchTessellationCompute(SDL_GPUDevice* device,
                                 SDL_GPUComputePipeline* pipeline,
                                 SDL_GPUTexture* displacementTexture,
                                 SDL_GPUSampler* displacementSampler,
                                 const TessellationGridParams& params,
                                 const TessellationGridBuffers& buffers);

/// Publishes `plane_{name}_vb`/`_ib` and the `plane_{name}` metadata JSON,
/// matching geometry.create_plane's convention.
void PublishTessellationGrid(WorkflowContext& context,
                             const TessellationGridParams& params,
                             const TessellationGridBuffers& buffers);

}  // namespace sdl3cpp::services::impl
