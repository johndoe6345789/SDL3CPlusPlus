#pragma once

#include "services/interfaces/workflow/racer/render/racer_scene_draw_step.hpp"

namespace sdl3cpp::services::impl {

/// The open render pass and the vertex uniforms being pushed with it.
struct RacerDrawPass {
    SDL_GPURenderPass* pass;
    SDL_GPUCommandBuffer* cmd;
    RacerVertexUniforms vertex;
};

/// Draws the model's opaque or blended batches with model matrix `m`.
/// Returns the number of draw calls issued.
int DrawRacerModel(RacerDrawPass& d, const RacerGpuModel& model,
                   const glm::mat4& m, bool blended);

/// Draws the skybox centred on `eye`, before anything else, with the
/// blended pipeline (no depth writes) so the world draws over it.
int DrawRacerSky(RacerDrawPass& d, const RacerWorldState& state,
                 SDL_GPUGraphicsPipeline* blend, const glm::vec3& eye);

/// A pod's cables (opaque pass) or binder, flames and ground shadow
/// (blended pass).
/// The rivals' pods and their effects, leaving out any close enough
/// to the camera eye to fill the screen.
int DrawRacerRivals(RacerDrawPass& d, const RacerWorldState& state,
                    const glm::vec3& eye, bool blended);

/// The track's hazards: blaster bolts, vents' plumes, falling rocks.
/// Blended pass only.
int DrawRacerHazards(RacerDrawPass& d, const RacerWorldState& state);

int DrawRacerPodEffects(RacerDrawPass& d, const RacerWorldState& state,
                        const RacerPodState& pod, float roll,
                        const RacerPodRig& rig, bool blended);

/// A pod's model matrix: its position, heading and bank.
glm::mat4 RacerPodMatrix(const RacerPodState& pod, float roll);

}  // namespace sdl3cpp::services::impl
