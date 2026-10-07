#pragma once

#include "services/interfaces/workflow/racer/data/racer_model.hpp"
#include "services/interfaces/workflow/racer/world/racer_gpu_types.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace sdl3cpp::services::impl {

/// What a pod needs for its effects, in its own engine-space frame.
struct RacerPodRig {
    RacerGpuModel cables;            ///< opaque: engines to cockpit
    RacerGpuModel binder;            ///< blended: the energy arc
    std::vector<glm::vec3> exhausts;
    float exhaustRadius = 0.5f;
    float reach = 0.f;   ///< metres from the pod's origin to its far end
    float front = 0.f;   ///< metres the engines reach ahead of the origin
    float back = 0.f;    ///< metres the cockpit trails behind it
    float halfWidth = 0.f;
    float length = 0.f;  ///< the footprint, front to back
};

/// Shared effect shapes, built once: a unit flame cone pointing along
/// +z (normal and boosting colours) and a unit shadow quad.
struct RacerEffectShapes {
    RacerGpuModel flame;
    RacerGpuModel boostFlame;
    RacerGpuModel shadow;
    RacerGpuModel bolt;   ///< a blaster bolt: unit length along +z
    RacerGpuModel rock;   ///< half a rough rock: a squat cone along +z
};

/// Builds a pod's cables and binder from where its parts were laid out.
/// `binderColour` tints the arc (each racer's own differs).
RacerPodRig BuildRacerPodRig(SDL_GPUDevice* device, const RacerModel& pod,
                             const RacerGpuTexture& white,
                             const glm::vec3& binderColour);

/// One untextured batch (the white texture, vertex colours) on the GPU.
RacerGpuModel UploadRacerShape(SDL_GPUDevice* device,
                               const RacerGpuTexture& white,
                               const std::vector<RacerGpuVertex>& vertices,
                               bool blended);

RacerEffectShapes BuildRacerEffectShapes(SDL_GPUDevice* device,
                                         const RacerGpuTexture& white);

/// Vertices for the shapes, kept apart from the upload so they can be
/// checked without a GPU.
std::vector<RacerGpuVertex> RacerConeVertices(int segments,
                                              const glm::vec4& base,
                                              const glm::vec4& tip);
std::vector<RacerGpuVertex> RacerBeamVertices(const glm::vec3& from,
                                              const glm::vec3& to,
                                              float width,
                                              const glm::vec4& colour);

}  // namespace sdl3cpp::services::impl
