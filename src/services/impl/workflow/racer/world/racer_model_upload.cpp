#include "services/interfaces/workflow/racer/world/racer_gpu_upload.hpp"

namespace sdl3cpp::services::impl {
namespace {

bool IsDecal(const RacerMaterialRef& m) {
    return m.format == RacerTextureFormat::Intensity4 ||
           m.format == RacerTextureFormat::Intensity8;
}

RacerGpuVertex ToGpu(const RacerModelVertex& v) {
    const glm::vec3 p = RacerToEngine(v.x, v.y, v.z);
    return {p.x, p.y, p.z, v.u, v.v, v.a / 255.f, 0.f,
            v.r / 255.f, v.g / 255.f, v.b / 255.f};
}

}  // namespace

RacerGpuModel UploadRacerModel(SDL_GPUDevice* device, RacerWorldState& state,
                               const RacerModel& model, RacerGround* ground) {
    RacerGpuModel out;
    for (const RacerModelBatch& batch : model.batches) {
        std::vector<RacerGpuVertex> vertices;
        vertices.reserve(batch.vertices.size());
        for (const RacerModelVertex& v : batch.vertices) {
            vertices.push_back(ToGpu(v));
        }
        const bool decal = IsDecal(batch.material);
        if (ground && !decal) {
            for (std::size_t i = 0; i + 2 < vertices.size(); i += 3) {
                const auto& a = vertices[i];
                const auto& b = vertices[i + 1];
                const auto& c = vertices[i + 2];
                AddRacerGroundTriangle(*ground, {a.x, a.y, a.z},
                                       {b.x, b.y, b.z}, {c.x, c.y, c.z});
            }
        }
        RacerGpuBatch gpu;
        gpu.vertices = UploadRacerVertexBuffer(device, vertices);
        gpu.vertexCount = static_cast<std::uint32_t>(vertices.size());
        const RacerGpuTexture texture =
            AcquireRacerTexture(device, state, batch.material);
        gpu.texture = texture.texture;
        gpu.sampler = texture.sampler;
        gpu.blended = decal;
        if (gpu.vertices) out.batches.push_back(gpu);
    }
    return out;
}

void ReleaseRacerWorld(SDL_GPUDevice* device, RacerWorldState& state) {
    if (device) {
        for (RacerGpuModel* model : {&state.trackModel, &state.podModel}) {
            for (const RacerGpuBatch& batch : model->batches) {
                SDL_ReleaseGPUBuffer(device, batch.vertices);
            }
        }
        state.textures[~0ull] = state.white;
        for (const auto& [key, texture] : state.textures) {
            if (texture.sampler) SDL_ReleaseGPUSampler(device, texture.sampler);
            if (texture.texture) SDL_ReleaseGPUTexture(device, texture.texture);
        }
    }
    state.trackModel = RacerGpuModel{};
    state.podModel = RacerGpuModel{};
    state.textures.clear();
    state.white = RacerGpuTexture{};
    state.loaded = false;
}

}  // namespace sdl3cpp::services::impl
