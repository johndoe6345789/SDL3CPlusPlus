#include "services/interfaces/workflow/rendering/draw_textured_resources.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

void BindDrawTexturedSamplers(SDL_GPURenderPass* pass,
                              const WorkflowContext& context,
                              SDL_GPUTexture* texture,
                              SDL_GPUSampler* sampler) {
    auto* shadow_tex =
        context.Get<SDL_GPUTexture*>("shadow_depth_texture", nullptr);
    auto* shadow_samp =
        context.Get<SDL_GPUSampler*>("shadow_depth_sampler", nullptr);
    if (shadow_tex && shadow_samp) {
        SDL_GPUTextureSamplerBinding bindings[2] = {};
        bindings[0].texture                      = texture;
        bindings[0].sampler                      = sampler;
        bindings[1].texture                      = shadow_tex;
        bindings[1].sampler                      = shadow_samp;
        SDL_BindGPUFragmentSamplers(pass, 0, bindings, 2);
    } else {
        SDL_GPUTextureSamplerBinding tex_binding = {};
        tex_binding.texture                      = texture;
        tex_binding.sampler                      = sampler;
        SDL_BindGPUFragmentSamplers(pass, 0, &tex_binding, 1);
    }
}

bool ResolveDrawTexturedResources(const WorkflowContext& context,
                                  const DrawTexturedParams& params,
                                  const std::shared_ptr<ILogger>& logger,
                                  DrawTexturedResources& out) {
    out.vb = context.Get<SDL_GPUBuffer*>("plane_" + params.meshName + "_vb",
                                         nullptr);
    out.ib = context.Get<SDL_GPUBuffer*>("plane_" + params.meshName + "_ib",
                                         nullptr);
    const auto* mesh_meta =
        context.TryGet<nlohmann::json>("plane_" + params.meshName);
    if (!out.vb || !out.ib || !mesh_meta) {
        if (logger) {
            logger->Warn("draw.textured: Mesh '" + params.meshName +
                         "' not found in context");
        }
        return false;
    }
    out.indexCount = (*mesh_meta)["index_count"];

    out.texture =
        context.Get<SDL_GPUTexture*>(params.textureName + "_gpu", nullptr);
    out.sampler =
        context.Get<SDL_GPUSampler*>(params.textureName + "_sampler", nullptr);
    if (!out.texture || !out.sampler) {
        if (logger) {
            logger->Warn("draw.textured: Texture '" + params.textureName +
                         "' not found in context");
        }
        return false;
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
