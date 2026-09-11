#include "services/interfaces/workflow/gta5/gta5_ped_frame.hpp"

#include "services/interfaces/workflow/gta5/gta5_texture_upload.hpp"
#include "services/interfaces/workflow/graphics/texture_load_sampler.hpp"

namespace sdl3cpp::services::impl {

bool UploadGta5PedPart(Gta5StreamState& state, SDL_GPUDevice* device,
                       const Gta5Resource& ytd, Gta5PedPart& part,
                       Gta5SubMesh& sub, std::vector<SDL_GPUTexture*>& owned) {
    const Gta5SubMeshData& m = part.mesh;
    const auto vertices = static_cast<std::uint32_t>(m.vertices.size());
    const auto indices = static_cast<std::uint32_t>(m.indices.size());
    for (Gta5ArenaSlot& slot : part.ring) {
        if (!state.arena.Upload(device, state.uploads, m.vertices.data(),
                                vertices, m.indices.data(), indices, slot)) {
            return false;
        }
    }
    sub.slot = part.ring[0];
    sub.indexCount = indices;
    sub.surface = {1.f, 1.f, 1.f, m.alphaCutoff};
    sub.blend = m.blend;
    const Gta5TextureBlob blob = ReadGta5DictionaryTexture(ytd, m.textureHash);
    const Gta5GpuTexture gpu =
        UploadGta5TextureBlob(blob, device, state.uploads);
    // The ped loads on the first frame, before any map texture has made
    // the shared sampler; without one it wore the placeholder bricks.
    if (gpu.texture && !state.textureSampler) {
        state.textureSampler = CreateTextureLoadSampler(device, nullptr, 16,
                                                        0.f);
    }
    if (gpu.texture && state.textureSampler) {
        owned.push_back(gpu.texture);
        sub.texture = gpu.texture;
        sub.sampler = state.textureSampler;
        sub.textureSize = {blob.width, blob.height};
    }
    return true;
}

void StageGta5PedFrame(Gta5StreamState& state, SDL_GPUDevice* device,
                       Gta5Ped& ped, const std::vector<glm::mat4>& skin,
                       int ring, std::vector<BspRenderVertex>& scratch) {
    for (std::size_t i = 0; i < ped.parts.size(); ++i) {
        const Gta5ArenaSlot& slot = ped.parts[i].ring[ring];
        SkinGta5PedPart(ped.parts[i], skin, scratch);
        state.uploads.StageBuffer(
            device, scratch.data(),
            static_cast<std::uint32_t>(scratch.size() * sizeof(scratch[0])),
            state.arena.Vertices(slot.block),
            slot.vertexOffset * Gta5GeometryArena::kVertexStride);
        ped.geometry.subMeshes[i].slot = slot;
    }
}

}  // namespace sdl3cpp::services::impl
