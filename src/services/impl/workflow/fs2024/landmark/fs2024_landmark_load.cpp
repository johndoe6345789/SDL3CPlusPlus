#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_load.hpp"

#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_texture.hpp"
#include "services/interfaces/workflow/rendering/bsp_geometry_upload.hpp"

#include <map>

namespace sdl3cpp::services::impl {

Fs2024LandmarkKitGpu UploadFs2024LandmarkKit(SDL_GPUDevice* device,
                                             const Fs2024LandmarkMesh& mesh) {
    Fs2024LandmarkKitGpu kit;
    kit.bounds = mesh.bounds;
    std::map<std::string, SDL_GPUTextureSamplerBinding> maps;
    for (const auto& [uri, blocks] : mesh.textures) {
        const auto map = UploadFs2024LandmarkTexture(device, *blocks);
        if (!map.texture) continue;
        maps.emplace(uri, map);
        kit.textures.push_back(map.texture);
        kit.samplers.push_back(map.sampler);
    }
    for (const auto& prim : mesh.primitives) {
        if (prim.mesh.indices.empty()) continue;
        Fs2024LandmarkGroupGpu group;
        const BspGeometryBuffers buffers = UploadBspGeometryBuffers(
            device, prim.mesh.vertices, prim.mesh.indices);
        group.vertexBuffer = buffers.vertex_buffer;
        group.indexBuffer = buffers.index_buffer;
        group.indexCount =
            static_cast<std::uint32_t>(prim.mesh.indices.size());
        const auto map = maps.find(prim.baseColorImageUri);
        if (map != maps.end()) {
            group.texture = map->second.texture;
            group.sampler = map->second.sampler;
        }
        kit.groups.push_back(group);
    }
    return kit;
}

void ReleaseFs2024LandmarkKitGpu(SDL_GPUDevice* device,
                                 Fs2024LandmarkKitGpu& kit) {
    if (device) {
        for (const Fs2024LandmarkGroupGpu& group : kit.groups) {
            SDL_ReleaseGPUBuffer(device, group.vertexBuffer);
            SDL_ReleaseGPUBuffer(device, group.indexBuffer);
        }
        for (SDL_GPUTexture* texture : kit.textures) {
            SDL_ReleaseGPUTexture(device, texture);
        }
        for (SDL_GPUSampler* sampler : kit.samplers) {
            SDL_ReleaseGPUSampler(device, sampler);
        }
    }
    kit = {};
}

}  // namespace sdl3cpp::services::impl
