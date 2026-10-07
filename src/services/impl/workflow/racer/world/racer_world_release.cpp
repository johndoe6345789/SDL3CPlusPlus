#include "services/interfaces/workflow/racer/world/racer_gpu_upload.hpp"

namespace sdl3cpp::services::impl {

void ReleaseRacerWorld(SDL_GPUDevice* device, RacerWorldState& state) {
    if (device) {
        std::vector<RacerGpuModel*> models{
            &state.trackModel, &state.podModel, &state.skyModel};
        for (RacerOpponent& opponent : state.opponents) {
            models.insert(models.end(), {&opponent.model,
                                         &opponent.rig.cables,
                                         &opponent.rig.binder});
        }
        models.insert(models.end(),
                      {&state.podRig.cables, &state.podRig.binder,
                       &state.effects.flame, &state.effects.boostFlame,
                       &state.effects.shadow, &state.effects.bolt,
                       &state.effects.rock});
        for (RacerGpuModel* model : models) {
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
    state.skyModel = RacerGpuModel{};
    state.podRig = RacerPodRig{};
    state.effects = RacerEffectShapes{};
    state.opponents.clear();
    state.hazards.clear();
    state.ground = RacerGround{};
    state.lapPoints.clear();
    state.race = RacerRaceState{};
    state.textures.clear();
    state.white = RacerGpuTexture{};
    state.loaded = false;
}

}  // namespace sdl3cpp::services::impl
