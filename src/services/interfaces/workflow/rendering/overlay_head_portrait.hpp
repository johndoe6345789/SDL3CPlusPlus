#pragma once

#include "services/interfaces/workflow/rendering/overlay_sw_end_resources.hpp"

namespace sdl3cpp::services::impl {

/**
 * @brief Blits a 3D-rendered head portrait onto the overlay's face-rect area.
 *
 * Opens its own LOADOP_LOAD render pass on `swapchain` (so whatever the
 * overlay quad already drew is preserved), using the SW overlay's pipeline
 * (`res.pipeline`) with `headTexture` bound instead of the overlay texture.
 * Lazily creates `res.headSampler`/`res.headVertices` on first use.
 *
 * @param faceRectX/Y/W/H  Face rect in the fixed overlay pixel space (see
 *                          q3.hud_head_render, which computes them), sized
 *                          `overlayWidth` x `overlayHeight`.
 * @return false if the pass could not be opened or a lazy resource failed to
 *         create; the caller has nothing further to do either way.
 */
bool BlitHeadPortrait(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* swapchain,
                      OverlaySwEndResources& res, SDL_GPUTexture* headTexture,
                      float faceRectX, float faceRectY, float faceRectW,
                      float faceRectH, int overlayWidth, int overlayHeight);

}  // namespace sdl3cpp::services::impl
