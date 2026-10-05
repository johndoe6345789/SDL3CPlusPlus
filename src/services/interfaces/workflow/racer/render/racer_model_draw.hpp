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

/// The pod's model matrix: its position, heading and bank.
glm::mat4 RacerPodMatrix(const RacerWorldState& state);

}  // namespace sdl3cpp::services::impl
