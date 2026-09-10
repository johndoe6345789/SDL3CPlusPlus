#pragma once

#include "services/interfaces/workflow/quake3/q3_sky_resources.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/**
 * @brief Builds the sky dome, uploads it, and readies its scroll buffer.
 *
 * Fills in `out`'s vertex/index buffers and index count, keeps the
 * unscrolled vertices for UpdateSkyScroll() to offset each frame, and
 * creates the transfer buffer those per-frame uploads reuse.
 */
bool UploadSkyDome(SDL_GPUDevice* device, SkyResources& out);

}  // namespace sdl3cpp::services::impl
