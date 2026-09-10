#include "services/interfaces/workflow/rendering/draw_legacy_map_geometry.hpp"
#include "services/interfaces/workflow/rendering/legacy_mesh_normal.hpp"

namespace sdl3cpp::services::impl {

namespace {

/// Binds one legacy mesh's VB/IB/textures/uniforms and draws it, or does
/// nothing if its buffers or texture aren't loaded.
void DrawLegacyMesh(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                    WorkflowContext& context, const nlohmann::json& node,
                    rendering::VertexUniformData vu,
                    const rendering::FragmentUniformData& fu,
                    const DrawMapTextureConfig& config,
                    SDL_GPUTexture* shadowTex, SDL_GPUSampler* shadowSamp) {
    std::string meshName = node["name"];
    auto* vb =
        context.Get<SDL_GPUBuffer*>("plane_" + meshName + "_vb", nullptr);
    auto* ib =
        context.Get<SDL_GPUBuffer*>("plane_" + meshName + "_ib", nullptr);
    if (!vb || !ib) return;

    std::string texKey = ResolveLegacyMeshTexture(meshName, config);
    auto* meshTex  = context.Get<SDL_GPUTexture*>(texKey + "_gpu", nullptr);
    auto* meshSamp = context.Get<SDL_GPUSampler*>(texKey + "_sampler",
                                                   nullptr);
    if (!meshTex || !meshSamp) return;

    SDL_GPUTextureSamplerBinding bindings[2] = {};
    bindings[0].texture = meshTex;
    bindings[0].sampler = meshSamp;
    bindings[1].texture = shadowTex ? shadowTex : meshTex;
    bindings[1].sampler = shadowSamp ? shadowSamp : meshSamp;
    SDL_BindGPUFragmentSamplers(pass, 0, bindings, 2);

    SDL_GPUBufferBinding vbBind = {};
    vbBind.buffer               = vb;
    SDL_BindGPUVertexBuffers(pass, 0, &vbBind, 1);
    SDL_GPUBufferBinding ibBind = {};
    ibBind.buffer               = ib;
    SDL_BindGPUIndexBuffer(pass, &ibBind, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    ApplyLegacyMeshNormal(node, vu.normal);

    SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));
    SDL_PushGPUFragmentUniformData(cmd, 0, &fu, sizeof(fu));
    uint32_t indexCount = node["index_count"];
    SDL_DrawGPUIndexedPrimitives(pass, indexCount, 1, 0, 0, 0);
}

}  // namespace

void DrawLegacyMapGeometry(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                           WorkflowContext& context,
                           const nlohmann::json& mapNodes,
                           rendering::VertexUniformData vu,
                           const rendering::FragmentUniformData& fu,
                           const DrawMapTextureConfig& config) {
    auto* shadowTex =
        context.Get<SDL_GPUTexture*>("shadow_depth_texture", nullptr);
    auto* shadowSamp =
        context.Get<SDL_GPUSampler*>("shadow_depth_sampler", nullptr);

    for (const auto& node : mapNodes) {
        DrawLegacyMesh(pass, cmd, context, node, vu, fu, config, shadowTex,
                       shadowSamp);
    }
}

}  // namespace sdl3cpp::services::impl
