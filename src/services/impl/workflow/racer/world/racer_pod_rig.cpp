#include "services/interfaces/workflow/racer/world/racer_pod_rig.hpp"

#include "services/interfaces/workflow/racer/world/racer_gpu_upload.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

RacerGpuModel UploadRacerShape(SDL_GPUDevice* device,
                               const RacerGpuTexture& white,
                               const std::vector<RacerGpuVertex>& vertices,
                               bool blended) {
    RacerGpuModel model;
    RacerGpuBatch batch;
    batch.vertices = UploadRacerVertexBuffer(device, vertices);
    batch.vertexCount = static_cast<std::uint32_t>(vertices.size());
    batch.texture = white.texture;
    batch.sampler = white.sampler;
    batch.blended = blended;
    if (batch.vertices) model.batches.push_back(batch);
    return model;
}

namespace {

glm::vec3 Engine(const std::array<float, 3>& p) {
    return RacerToEngine(p[0], p[1], p[2]);
}

}  // namespace

RacerPodRig BuildRacerPodRig(SDL_GPUDevice* device, const RacerModel& pod,
                             const RacerGpuTexture& white,
                             const glm::vec3& binderColour) {
    RacerPodRig rig;
    const glm::vec3 cockpit = Engine(pod.cockpitFront);
    std::vector<RacerGpuVertex> cables;
    for (const auto& exhaust : pod.engineExhausts) {
        rig.exhausts.push_back(Engine(exhaust));
        const auto beam = RacerBeamVertices(rig.exhausts.back(), cockpit,
                                            0.12f, {0.15f, 0.14f, 0.13f, 1.f});
        cables.insert(cables.end(), beam.begin(), beam.end());
    }
    rig.exhaustRadius = 0.45f * pod.engineRadius * kRacerWorldScale;
    for (const RacerModelBatch& batch : pod.batches) {
        for (const RacerModelVertex& v : batch.vertices) {
            rig.reach = std::max(rig.reach, std::hypot(v.x, v.y));
        }
    }
    rig.reach *= kRacerWorldScale;
    rig.cables = UploadRacerShape(device, white, cables, false);
    if (rig.exhausts.size() >= 2) {
        // The binder arcs between the engines' inner faces, a little
        // ahead of the exhausts.
        const glm::vec3 lift(0.f, 0.f, 0.3f * glm::length(
            rig.exhausts[1] - rig.exhausts[0]));
        rig.binder = UploadRacerShape(
            device, white,
            RacerBeamVertices(rig.exhausts[0] + lift, rig.exhausts[1] + lift,
                              0.3f, glm::vec4(binderColour, 0.55f)),
            true);
    }
    return rig;
}

}  // namespace sdl3cpp::services::impl
