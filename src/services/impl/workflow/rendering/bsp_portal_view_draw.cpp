#include "services/interfaces/workflow/rendering/bsp_portal_view_draw.hpp"

#include <string>

namespace sdl3cpp::services::impl {

bool DrawPortalViewGeometry(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                            WorkflowContext& context,
                            const nlohmann::json& mapNodes,
                            SDL_GPUGraphicsPipeline* pipeline,
                            SDL_GPUTexture* lmTex, SDL_GPUSampler* lmSamp,
                            const rendering::VertexUniformData& vu,
                            const rendering::FragmentUniformData& fu) {
    const auto& firstNode      = mapNodes[0];
    const std::string meshName = firstNode["name"];
    auto* vb =
        context.Get<SDL_GPUBuffer*>("plane_" + meshName + "_vb", nullptr);
    auto* ib =
        context.Get<SDL_GPUBuffer*>("plane_" + meshName + "_ib", nullptr);
    if (!vb || !ib) return false;

    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_GPUBufferBinding vbBind = {};
    vbBind.buffer               = vb;
    SDL_BindGPUVertexBuffers(pass, 0, &vbBind, 1);
    SDL_GPUBufferBinding ibBind = {};
    ibBind.buffer               = ib;
    SDL_BindGPUIndexBuffer(pass, &ibBind, SDL_GPU_INDEXELEMENTSIZE_32BIT);
    SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));
    SDL_PushGPUFragmentUniformData(cmd, 0, &fu, sizeof(fu));

    for (const auto& node : mapNodes) {
        const int texIdx           = node.value("texture_index", -1);
        SDL_GPUTexture* albedoTex  = nullptr;
        SDL_GPUSampler* albedoSamp = nullptr;
        if (texIdx >= 0) {
            const std::string texKey = "bsp_tex_" + std::to_string(texIdx);
            albedoTex = context.Get<SDL_GPUTexture*>(texKey + "_gpu", nullptr);
            albedoSamp =
                context.Get<SDL_GPUSampler*>(texKey + "_sampler", nullptr);
        }
        if (!albedoTex || !albedoSamp) continue;

        SDL_GPUTextureSamplerBinding bindings[4] = {};
        bindings[0].texture                      = albedoTex;
        bindings[0].sampler                      = albedoSamp;
        bindings[1].texture                      = albedoTex;
        bindings[1].sampler                      = albedoSamp;
        bindings[2].texture                      = lmTex;
        bindings[2].sampler                      = lmSamp;
        bindings[3].texture                      = albedoTex;
        bindings[3].sampler                      = albedoSamp;
        SDL_BindGPUFragmentSamplers(pass, 0, bindings, 4);

        SDL_DrawGPUIndexedPrimitives(pass, node["index_count"].get<uint32_t>(),
                                     1, node.value("index_offset", 0u), 0, 0);
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
