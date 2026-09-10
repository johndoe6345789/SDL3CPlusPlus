#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Returns the 1x1 solid-color GPU texture+sampler cached under
 *        `key` in `context`, creating and caching it on first use.
 *
 * The color is stored with alpha 230 (a soft translucency for pickup
 * billboards). Returns the existing texture if `key + "_gpu"` is already
 * set, ignoring `r`/`g`/`b` in that case.
 */
SDL_GPUTexture* EnsurePickupColorTexture(SDL_GPUDevice* device,
                                         WorkflowContext& context,
                                         const std::string& key, uint8_t r,
                                         uint8_t g, uint8_t b);

}  // namespace sdl3cpp::services::impl
