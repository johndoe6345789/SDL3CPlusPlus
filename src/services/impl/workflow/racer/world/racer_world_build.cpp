#include "services/interfaces/workflow/racer/world/racer_world_build.hpp"

#include "services/interfaces/workflow/racer/world/racer_gpu_upload.hpp"

#include <cmath>
#include <string>

namespace sdl3cpp::services::impl {

void PlaceRacerPodOnLap(RacerWorldState& state, int index, float speed) {
    const int count = static_cast<int>(state.lapPoints.size());
    if (count < 2) return;
    index = ((index % count) + count) % count;
    const glm::vec3 at = state.lapPoints[index];
    const glm::vec3 to = state.lapPoints[(index + 1) % count] - at;
    RacerPodState& pod = state.pod;
    pod = RacerPodState{};
    pod.heading = std::atan2(to.x, -to.z);
    pod.speed = speed;
    const auto floor = RacerGroundHeight(state.ground, at.x, at.z,
                                         at.y + state.podSpec.stepHeight);
    pod.position = at;
    pod.position.y = (floor ? *floor : at.y) + state.podSpec.hoverHeight;
}

bool BuildRacerWorld(SDL_GPUDevice* device, RacerWorldState& state,
                     const std::shared_ptr<ILogger>& logger) {
    const RacerModel track = LoadRacerModel(state.library, state.track.model);
    const RacerModel pod = LoadRacerModel(state.library, state.racer.podd);
    state.spline = ReadRacerSpline(
        RacerSplineBytes(state.library, state.track.spline));
    state.lap = RacerSplineMainLoop(state.spline);
    if (!track.valid || state.lap.size() < 2) {
        if (logger) {
            logger->Error("racer.world.load: '" + state.track.name +
                          "' has no geometry or no closed lap");
        }
        return false;
    }
    state.lapPoints.clear();
    for (int i : state.lap) {
        const RacerVec3& k = state.spline[i].knot;
        state.lapPoints.push_back(RacerToEngine(k.x, k.y, k.z));
    }
    RacerTexture white;
    white.width = white.height = 4;
    white.rgba.assign(4 * 4 * 4, 255);
    state.white = UploadRacerTexture(device, white, 1);
    state.trackModel = UploadRacerModel(device, state, track, &state.ground);
    state.podModel = UploadRacerModel(device, state, pod, nullptr);
    PlaceRacerPodOnLap(state, 0, 0.f);
    if (logger) {
        logger->Info("racer.world.load: " + state.track.name + ", " +
                     std::to_string(track.triangleCount) + " triangles in " +
                     std::to_string(state.trackModel.batches.size()) +
                     " batches, " + std::to_string(state.textures.size()) +
                     " textures at " + std::to_string(state.textureScale) +
                     "x, lap of " + std::to_string(state.lap.size()) +
                     " segments; pod " + state.racer.name + " (" +
                     std::to_string(pod.triangleCount) + " triangles)");
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
