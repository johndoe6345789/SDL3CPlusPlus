#pragma once

#include "services/interfaces/workflow/racer/data/racer_asset_library.hpp"
#include "services/interfaces/workflow/racer/data/racer_spline.hpp"
#include "services/interfaces/workflow/racer/data/racer_track_table.hpp"
#include "services/interfaces/workflow/racer/player/racer_pod_physics.hpp"
#include "services/interfaces/workflow/racer/world/racer_ground.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <map>
#include <vector>

namespace sdl3cpp::services::impl {

/// Game units to engine units. The game is z-up in units of about 5 cm
/// (a pod is ~120 units long), the engine y-up in metres-ish.
constexpr float kRacerWorldScale = 0.05f;

/// A game-space point in engine space: (x, z, -y), scaled.
inline glm::vec3 RacerToEngine(float x, float y, float z) {
    return glm::vec3(x, z, -y) * kRacerWorldScale;
}

/// One uploaded material batch: an unindexed triangle list.
struct RacerGpuBatch {
    SDL_GPUBuffer* vertices = nullptr;
    std::uint32_t vertexCount = 0;
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
    bool blended = false;  ///< intensity decals (shadows, glows)
};

struct RacerGpuModel {
    std::vector<RacerGpuBatch> batches;
};

/// A GPU texture made from one material, upscaled once and shared.
struct RacerGpuTexture {
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
};

/// Lap progress for the player's pod.
struct RacerRaceState {
    int lapsTotal = 3;
    int lap = 1;                 ///< 1-based; lapsTotal + 1 once finished
    int segment = -1;            ///< nearest lap point, -1 before start
    float raceTime = 0.f;
    float lapTime = 0.f;
    float bestLap = 0.f;         ///< 0 until a lap is completed
    float countdown = 3.f;       ///< seconds before the start
    bool finished = false;
};

/// Everything the racer.* steps share for one loaded race.
struct RacerWorldState {
    RacerAssetLibrary library;
    RacerTrackInfo track;
    RacerPodInfo racer;            ///< who is flying, and their pod model
    RacerGpuModel trackModel;
    RacerGpuModel podModel;
    std::map<std::uint64_t, RacerGpuTexture> textures;
    RacerGpuTexture white;   ///< for meshes shaded by vertex colour only
    std::vector<RacerSplineSegment> spline;
    std::vector<int> lap;              ///< main-loop segment order
    std::vector<glm::vec3> lapPoints;  ///< lap knots in engine space
    RacerGround ground;
    RacerPodSpec podSpec;
    RacerPodState pod;
    RacerRaceState race;
    float podRoll = 0.f;               ///< visual bank into turns
    int textureScale = 4;
    bool loaded = false;
};

}  // namespace sdl3cpp::services::impl
