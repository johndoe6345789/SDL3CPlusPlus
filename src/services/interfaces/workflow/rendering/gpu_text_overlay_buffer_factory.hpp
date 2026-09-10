#pragma once

#include "services/interfaces/workflow/rendering/gpu_text_overlay_resources.hpp"

namespace sdl3cpp::services::impl {

/**
 * @brief Allocates the overlay's GPU-side texture, transfer buffer, vertex
 *        buffer, and sampler.
 *
 * Fills whichever fields succeed directly into `res` so a failed call still
 * leaves `res` in a state DestroyGpuTextOverlayResources() can clean up
 * (every field it touches is null-checked there).
 *
 * @return Empty string on success, otherwise the reason allocation failed.
 */
const char* CreateOverlayGpuBuffers(SDL_GPUDevice* device,
                                    GpuTextOverlayResources& res);

}  // namespace sdl3cpp::services::impl
