#pragma once

#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Everything one frame's instance draws need, gathered once by the step
/// so the inner loop touches no workflow context.
struct Gta5DrawContext {
    SDL_GPURenderPass* pass{nullptr};
    SDL_GPUCommandBuffer* cmd{nullptr};
    glm::mat4 view{1.f};
    glm::mat4 proj{1.f};
    glm::mat4 shadowVP{1.f};
    glm::vec3 cameraPos{0.f};
    rendering::FragmentUniformData fragUniforms{};
    SDL_GPUTexture* texture{nullptr};
    SDL_GPUSampler* sampler{nullptr};
    /// Alpha-blended, no depth write: decals and glass, drawn last.
    SDL_GPUGraphicsPipeline* blendPipeline{nullptr};
    /// Four samplers, blended by vertex colour: GTA's layered terrain.
    SDL_GPUGraphicsPipeline* terrainPipeline{nullptr};
    SDL_GPUTexture* shadowTexture{nullptr};
    SDL_GPUSampler* shadowSampler{nullptr};
};

/// Draw the batch gta5.tiles.cull built, instanced: one call per
/// archetype material. Returns the draw count.
int DrawGta5Instances(const Gta5StreamState& state,
                      const Gta5DrawContext& draw,
                      int* textureBinds = nullptr);

}  // namespace sdl3cpp::services::impl
