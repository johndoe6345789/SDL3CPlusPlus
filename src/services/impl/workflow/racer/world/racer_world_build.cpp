#include "services/interfaces/workflow/racer/world/racer_world_build.hpp"

#include "services/interfaces/workflow/racer/world/racer_gpu_upload.hpp"

#include <cmath>
#include <string>

namespace sdl3cpp::services::impl {

void PlaceRacerPod(const RacerWorldState& state, RacerPodState& pod,
                   int index, float speed, float lateral) {
    const int count = static_cast<int>(state.lapPoints.size());
    if (count < 2) return;
    index = ((index % count) + count) % count;
    const glm::vec3 to =
        state.lapPoints[(index + 1) % count] - state.lapPoints[index];
    pod = RacerPodState{};
    pod.heading = std::atan2(to.x, -to.z);
    pod.speed = speed;
    const glm::vec3 right(std::cos(pod.heading), 0.f, std::sin(pod.heading));
    const glm::vec3 at = state.lapPoints[index] + right * lateral;
    const auto floor = RacerGroundHeight(state.ground, at.x, at.z,
                                         at.y + state.podSpec.stepHeight);
    pod.position = at;
    pod.position.y = (floor ? *floor : at.y) + state.podSpec.hoverHeight;
}

void PlaceRacerPodOnLap(RacerWorldState& state, int index, float speed) {
    PlaceRacerPod(state, state.pod, index, speed, 0.f);
}

bool BuildRacerWorld(SDL_GPUDevice* device, RacerWorldState& state,
                     const std::shared_ptr<ILogger>& logger) {
    const RacerModel track = LoadRacerModel(state.library, state.track.model);
    const RacerModel pod = LoadRacerModel(state.library, state.racer.podd,
                                          RacerModelScope::PodParts);
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
    BuildRacerLapPoints(state);
    RacerTexture white;
    white.width = white.height = 4;
    white.rgba.assign(4 * 4 * 4, 255);
    state.white = UploadRacerTexture(device, white, 1);
    const bool collision = BuildRacerCollisionGround(state, track);
    state.trackModel = UploadRacerModel(device, state, track,
                                        collision ? nullptr : &state.ground);
    state.podModel = UploadRacerModel(device, state, pod, nullptr,
                                      RacerVertexShading::Normal);
    PlaceRacerPodOnLap(state, 0, 0.f);
    TraceModelExtent(logger, "pod", pod);
    if (logger) {
        logger->Info("racer.world.load: " + state.track.name + ", " +
                     std::to_string(track.triangleCount) + " triangles in " +
                     std::to_string(state.trackModel.batches.size()) +
                     " batches, " + std::to_string(state.textures.size()) +
                     " textures at " + std::to_string(state.textureScale) +
                     "x, lap of " + std::to_string(state.lap.size()) +
                     " segments, " +
                     std::to_string(state.ground.triangles.size() / 3) +
                     " floor and " +
                     std::to_string(state.ground.walls.size() / 3) +
                     " wall triangles; pod " + state.racer.name + " (" +
                     std::to_string(pod.triangleCount) + " triangles)");
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
