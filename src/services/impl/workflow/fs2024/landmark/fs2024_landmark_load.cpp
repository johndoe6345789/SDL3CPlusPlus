#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_load.hpp"

#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_place.hpp"
#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_texture.hpp"
#include "services/interfaces/workflow/rendering/bsp_geometry_upload.hpp"

namespace sdl3cpp::services::impl {
namespace {

namespace f = sdl3cpp::fs2024;

void Grow(Fs2024LandmarkKitGpu& kit, const f::GltfPrimitive& prim) {
    for (const BspRenderVertex& v : prim.mesh.vertices) {
        kit.min = glm::min(kit.min, glm::vec3(v.x, v.y, v.z));
        kit.max = glm::max(kit.max, glm::vec3(v.x, v.y, v.z));
    }
}

}  // namespace

Fs2024LandmarkKitGpu LoadFs2024LandmarkKit(
    SDL_GPUDevice* device, const std::string& library,
    const f::ModelLibraryEntry& entry, const std::string& texturesDir,
    std::size_t lodBudgetBytes) {
    const auto riff = f::ReadModelRiff(library, entry);
    const auto lods = f::ListModelRiffLods(riff);
    Fs2024LandmarkKitGpu kit;
    if (lods.empty()) return kit;
    const f::GltfLod lod = f::ParseModelRiffLod(
        riff, ChooseFs2024LandmarkLod(lods, lodBudgetBytes));
    kit.min = glm::vec3(1e30f);
    kit.max = glm::vec3(-1e30f);
    for (const f::GltfPrimitive& prim : lod.primitives) {
        if (prim.mesh.indices.empty()) continue;
        Grow(kit, prim);
        Fs2024LandmarkGroupGpu group;
        const BspGeometryBuffers buffers = UploadBspGeometryBuffers(
            device, prim.mesh.vertices, prim.mesh.indices);
        group.vertexBuffer = buffers.vertex_buffer;
        group.indexBuffer = buffers.index_buffer;
        group.indexCount =
            static_cast<std::uint32_t>(prim.mesh.indices.size());
        if (!prim.baseColorImageUri.empty()) {
            UploadFs2024LandmarkTexture(
                device, texturesDir + "/" + prim.baseColorImageUri, group);
        }
        kit.groups.push_back(group);
    }
    if (kit.groups.empty()) kit.min = kit.max = glm::vec3(0.f);
    return kit;
}

void ReleaseFs2024LandmarkKitGpu(SDL_GPUDevice* device,
                                 Fs2024LandmarkKitGpu& kit) {
    if (!device) return;
    for (Fs2024LandmarkGroupGpu& group : kit.groups) {
        SDL_ReleaseGPUBuffer(device, group.vertexBuffer);
        SDL_ReleaseGPUBuffer(device, group.indexBuffer);
        SDL_ReleaseGPUTexture(device, group.texture);
        SDL_ReleaseGPUSampler(device, group.sampler);
    }
    kit.groups.clear();
}

}  // namespace sdl3cpp::services::impl
