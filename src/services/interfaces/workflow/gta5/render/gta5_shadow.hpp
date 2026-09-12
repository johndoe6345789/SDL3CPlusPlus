#pragma once

#include "services/interfaces/workflow/gta5/stream/gta5_instance_batch.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// The sun's view of the ground around `focus`: an orthographic box
/// `radius` metres each way across the light and deep enough for hills
/// and towers upwind, looking along `lightDir` (the way the light
/// travels). The box moves in whole shadow-map texels, so shadow edges
/// hold still while the camera moves under them.
glm::mat4 Gta5SunShadowViewProj(const glm::vec3& lightDir,
                                const glm::vec3& focus, float radius,
                                int size);

/// Draw `batch` into the shadow map's pass with gpu_pipeline_gta5_shadow
/// bound. Glass and decals cast nothing; cutouts test their alpha, with
/// `blank` bound where there is no texture to read. Returns the draws.
int DrawGta5ShadowCasters(const Gta5StreamState& state,
                          const Gta5InstanceBatch& batch,
                          SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* pass,
                          const glm::mat4& lightViewProj,
                          SDL_GPUTexture* blank, SDL_GPUSampler* sampler);

}  // namespace sdl3cpp::services::impl
