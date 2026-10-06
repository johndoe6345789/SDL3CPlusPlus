#include "services/interfaces/workflow/racer/world/racer_world_build.hpp"

#include "services/interfaces/workflow/racer/world/racer_gpu_upload.hpp"
#include "services/interfaces/workflow/racer/world/racer_pod_rig.hpp"

#include <cmath>
#include <string>

namespace sdl3cpp::services::impl {

bool BuildRacerWorld(SDL_GPUDevice* device, RacerWorldState& state,
                     const std::shared_ptr<ILogger>& logger) {
    const RacerModel track = LoadRacerModel(
        state.library, state.track.model, RacerModelScope::TrackWithoutSky);
    const RacerModel sky = LoadRacerModel(state.library, state.track.model,
                                          RacerModelScope::SkyOnly);
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
    const bool collision = BuildRacerCollisionGround(state, track, logger);
    state.trackModel = UploadRacerModel(device, state, track,
                                        collision ? nullptr : &state.ground);
    state.podModel = UploadRacerModel(device, state, pod, nullptr,
                                      RacerVertexShading::Normal);
    state.skyModel = UploadRacerModel(device, state, sky, nullptr);
    state.effects = BuildRacerEffectShapes(device, state.white);
    state.podRig = BuildRacerPodRig(device, pod, state.white,
                                    RacerBinderColour(state.racer.name));
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
