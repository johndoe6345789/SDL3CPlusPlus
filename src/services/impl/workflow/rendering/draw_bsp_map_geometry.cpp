#include "services/interfaces/workflow/rendering/draw_bsp_map_geometry.hpp"
#include "services/interfaces/workflow/rendering/bsp_map_geometry_textures.hpp"
#include "services/interfaces/workflow/rendering/draw_map_texture_config.hpp"

namespace sdl3cpp::services::impl {

namespace {

/// Binds 4 fragment samplers (albedo, shadow, lightmap, portal destination),
/// pushes this group's fragment uniforms, and issues its indexed draw call.
void DrawBspTextureGroup(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                         WorkflowContext& context, const nlohmann::json& node,
                         const BspMapTextures& textures,
                         const rendering::FragmentUniformData& fu,
                         float elapsed) {
    uint32_t indexCount     = node["index_count"];
    uint32_t indexOffset    = node.value("index_offset", 0u);
    int texIdx              = node.value("texture_index", -1);
    std::string textureName = node.value("texture_name", std::string{});

    SDL_GPUTexture* albedoTex  = nullptr;
    SDL_GPUSampler* albedoSamp = nullptr;
    ResolveBspGroupAlbedo(context, texIdx, textures, albedoTex, albedoSamp);
    if (!albedoTex || !albedoSamp) return;

    SDL_GPUTextureSamplerBinding bindings[4] = {};
    BuildBspSamplerBindings(albedoTex, albedoSamp, textures, bindings);
    SDL_BindGPUFragmentSamplers(pass, 0, bindings, 4);

    auto groupFu        = fu;
    groupFu.material[1] = elapsed;
    groupFu.material[3] = IsPortalTexture(textureName) ? 1.0f : 0.0f;
    SDL_PushGPUFragmentUniformData(cmd, 0, &groupFu, sizeof(groupFu));
    SDL_DrawGPUIndexedPrimitives(pass, indexCount, 1, indexOffset, 0, 0);
}

}  // namespace

void DrawBspMapGeometry(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                        WorkflowContext& context,
                        const nlohmann::json& mapNodes,
                        const rendering::FragmentUniformData& fu,
                        const rendering::VertexUniformData& vu,
                        const std::string& defaultTexture) {
    const auto& firstNode      = mapNodes[0];
    const std::string meshName = firstNode["name"];
    auto* vb =
        context.Get<SDL_GPUBuffer*>("plane_" + meshName + "_vb", nullptr);
    auto* ib =
        context.Get<SDL_GPUBuffer*>("plane_" + meshName + "_ib", nullptr);
    if (!vb || !ib) return;

    // Bind VB + IB once; shared by every BSP texture group.
    SDL_GPUBufferBinding vbBind = {};
    vbBind.buffer               = vb;
    SDL_BindGPUVertexBuffers(pass, 0, &vbBind, 1);
    SDL_GPUBufferBinding ibBind = {};
    ibBind.buffer               = ib;
    SDL_BindGPUIndexBuffer(pass, &ibBind, SDL_GPU_INDEXELEMENTSIZE_32BIT);
    SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));

    const BspMapTextures textures =
        GatherBspMapTextures(context, defaultTexture);
    const float elapsed =
        static_cast<float>(context.GetDouble("frame.elapsed", 0.0));

    for (const auto& node : mapNodes) {
        DrawBspTextureGroup(pass, cmd, context, node, textures, fu, elapsed);
    }
}

}  // namespace sdl3cpp::services::impl
