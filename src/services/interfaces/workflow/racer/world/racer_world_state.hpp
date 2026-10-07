#pragma once

#include "services/interfaces/workflow/racer/data/racer_asset_library.hpp"
#include "services/interfaces/workflow/racer/data/racer_spline.hpp"
#include "services/interfaces/workflow/racer/data/racer_track_table.hpp"
#include "services/interfaces/workflow/racer/flow/racer_flow.hpp"
#include "services/interfaces/workflow/racer/player/racer_pod_physics.hpp"
#include "services/interfaces/workflow/racer/player/racer_race_state.hpp"
#include "services/interfaces/workflow/racer/world/racer_ground.hpp"
#include "services/interfaces/workflow/racer/world/racer_opponent.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <map>
#include <vector>

namespace sdl3cpp::services::impl {

/// Points sampled along each lap segment's Bezier curve.
constexpr int kRacerLapSamples = 8;

/// Game units to engine units. The game is z-up in units of about 10 cm
/// (tracks ~25 m wide, Boonta Classic ~8.7 km a lap, Anakin's engines
/// ~7 m), the engine y-up in metres.
constexpr float kRacerWorldScale = 0.1f;

/// A game-space point in engine space: (x, z, -y), scaled.
inline glm::vec3 RacerToEngine(float x, float y, float z) {
    return glm::vec3(x, z, -y) * kRacerWorldScale;
}

/// Everything the racer.* steps share for one loaded race.
struct RacerWorldState {
    RacerAssetLibrary library;
    RacerTrackInfo track;
    RacerPodInfo racer;            ///< who is flying, and their pod model
    RacerGpuModel trackModel;
    RacerGpuModel podModel;
    RacerGpuModel skyModel;          ///< drawn round the camera
    RacerPodRig podRig;              ///< the player's cables and binder
    RacerEffectShapes effects;       ///< flames and shadows for all
    glm::vec3 fogColour{0.78f, 0.70f, 0.58f};
    std::map<std::uint64_t, RacerGpuTexture> textures;
    RacerGpuTexture white;   ///< for meshes shaded by vertex colour only
    std::vector<RacerSplineSegment> spline;
    std::vector<int> lap;              ///< main-loop segment order
    std::vector<glm::vec3> lapPoints;  ///< dense lap polyline, engine space
    RacerGround ground;
    RacerPodSpec podSpec;
    RacerPodState pod;
    RacerRaceState race;
    float podRoll = 0.f;               ///< visual bank into turns
    std::vector<RacerOpponent> opponents;
    RacerFlow flow;                    ///< menus; kept across races
    RacerTrackTable table;
    int textureScale = 4;
    bool loaded = false;
};

}  // namespace sdl3cpp::services::impl
