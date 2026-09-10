#include "services/interfaces/workflow/rendering/viewmodel_draw.hpp"

namespace sdl3cpp::services::impl {

void BindViewmodelTexture(const WorkflowContext& context,
                          SDL_GPURenderPass* pass, const std::string& texName) {
    // Bind texture if specified, else use a default
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
    if (!texName.empty()) {
        texture = context.Get<SDL_GPUTexture*>(texName + "_gpu", nullptr);
        sampler = context.Get<SDL_GPUSampler*>(texName + "_sampler", nullptr);
    }
    // Fall back to floor texture or any available texture
    if (!texture) {
        texture = context.Get<SDL_GPUTexture*>("floor_texture_gpu", nullptr);
    }
    if (!sampler) {
        sampler =
            context.Get<SDL_GPUSampler*>("floor_texture_sampler", nullptr);
    }

    if (!texture || !sampler) return;

    auto* shadowTex =
        context.Get<SDL_GPUTexture*>("shadow_depth_texture", nullptr);
    auto* shadowSamp =
        context.Get<SDL_GPUSampler*>("shadow_depth_sampler", nullptr);
    if (shadowTex && shadowSamp) {
        SDL_GPUTextureSamplerBinding bindings[2] = {};
        bindings[0].texture                      = texture;
        bindings[0].sampler                      = sampler;
        bindings[1].texture                      = shadowTex;
        bindings[1].sampler                      = shadowSamp;
        SDL_BindGPUFragmentSamplers(pass, 0, bindings, 2);
    } else {
        SDL_GPUTextureSamplerBinding binding = {};
        binding.texture                      = texture;
        binding.sampler                      = sampler;
        SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
    }
}

}  // namespace sdl3cpp::services::impl
