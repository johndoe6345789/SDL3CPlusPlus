#pragma once

#include "services/interfaces/workflow/quake3/q3_sky_resources.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/**
 * @brief Scrolls the sky's cloud texture coordinates and re-uploads them.
 *
 * Ported from ioq3 tr_shade_calc.c RB_CalcScrollTexCoords(): the offset
 * is the scroll speed times the shader time, wrapped back into [0, 1) so
 * it cannot grow without bound, and added to each vertex's coordinates.
 * The shader's tcMod scale is applied after, matching the order the
 * stage lists them in.
 *
 * Quake scrolls the coordinates rather than turning the sky, and it
 * shows: rotating the dome sweeps the clouds around the zenith like a
 * fan instead of drifting them across the sky.
 *
 * Runs its own command buffer because a copy pass cannot be recorded
 * while the frame's render pass is open. It is submitted before the
 * frame's, so the upload lands before the draw that reads it.
 */
void UpdateSkyScroll(SDL_GPUDevice* device, SkyResources& sky, float elapsed);

}  // namespace sdl3cpp::services::impl
