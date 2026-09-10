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
    SDL_GPUTexture* shadowTexture{nullptr};
    SDL_GPUSampler* shadowSampler{nullptr};
};

/// Draw every instance of every resident tile. Returns the draw count.
int DrawGta5Instances(const Gta5StreamState& state,
                      const Gta5DrawContext& draw);

}  // namespace sdl3cpp::services::impl
