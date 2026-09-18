#include "services/interfaces/workflow/fs2024/terrain/fs2024_draw_landmarks.hpp"

namespace sdl3cpp::services::impl {
namespace {

/// The kit's bounds, placed and moved with its tile.
bool Visible(const Fs2024Frustum& frustum, const Fs2024LandmarkKitGpu& kit,
             const glm::mat4& model, const glm::vec3& offset) {
    glm::vec3 lo(1e30f), hi(-1e30f);
    for (int corner = 0; corner < 8; ++corner) {
        const glm::vec3 local((corner & 1) ? kit.max.x : kit.min.x,
                              (corner & 2) ? kit.max.y : kit.min.y,
                              (corner & 4) ? kit.max.z : kit.min.z);
        const glm::vec3 placed(model * glm::vec4(local, 1.f));
        lo = glm::min(lo, placed);
        hi = glm::max(hi, placed);
    }
    return Fs2024BoxVisible(frustum, lo + offset, hi + offset);
}

void DrawGroup(SDL_GPURenderPass* pass, const Fs2024LandmarkGroupGpu& group,
               SDL_GPUTextureSamplerBinding untextured) {
    const SDL_GPUTextureSamplerBinding texture =
        group.texture ? SDL_GPUTextureSamplerBinding{group.texture,
                                                     group.sampler}
                      : untextured;
    if (!texture.texture) return;
    SDL_BindGPUFragmentSamplers(pass, 0, &texture, 1);
    SDL_GPUBufferBinding vb{};
    vb.buffer = group.vertexBuffer;
    SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
    SDL_GPUBufferBinding ib{};
    ib.buffer = group.indexBuffer;
    SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_32BIT);
    SDL_DrawGPUIndexedPrimitives(pass, group.indexCount, 1, 0, 0, 0);
}

}  // namespace

void DrawFs2024TileLandmarks(
    SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
    const Fs2024LoadedTile& tile,
    const std::unordered_map<std::string, Fs2024LandmarkKitGpu>& kits,
    Fs2024TerrainVertexUniforms vertex,
    const Fs2024TerrainFragmentUniforms& fragment,
    const Fs2024Frustum& frustum, SDL_GPUTextureSamplerBinding untextured) {
    if (tile.landmarks.empty()) return;
    SDL_PushGPUFragmentUniformData(cmd, 0, &fragment, sizeof(fragment));
    vertex.originOffset = glm::vec4(tile.offset, 0.f);
    for (const Fs2024LandmarkInstance& instance : tile.landmarks) {
        const auto kit = kits.find(instance.entry.name);
        if (kit == kits.end() || kit->second.groups.empty()) continue;
        if (!Visible(frustum, kit->second, instance.model, tile.offset)) {
            continue;
        }
        vertex.model = instance.model;
        SDL_PushGPUVertexUniformData(cmd, 0, &vertex, sizeof(vertex));
        for (const Fs2024LandmarkGroupGpu& group : kit->second.groups) {
            DrawGroup(pass, group, untextured);
        }
    }
}

}  // namespace sdl3cpp::services::impl
