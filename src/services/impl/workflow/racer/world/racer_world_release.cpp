#include "services/interfaces/workflow/racer/world/racer_gpu_upload.hpp"

namespace sdl3cpp::services::impl {

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
