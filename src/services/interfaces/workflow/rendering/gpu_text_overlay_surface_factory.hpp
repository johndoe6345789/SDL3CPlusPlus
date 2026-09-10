#pragma once

#include "services/interfaces/workflow/rendering/gpu_text_overlay_resources.hpp"

namespace sdl3cpp::services::impl {

/**
 * @brief Creates the CPU-side surface and software renderer the overlay
 *        text is rasterized into before being uploaded to the GPU texture.
 *
 * Fills whichever fields succeed directly into `res`; see
 * CreateOverlayGpuBuffers() for why that is safe on partial failure.
 *
 * @return Empty string on success, otherwise the reason creation failed.
 */
const char* CreateOverlaySurfaceAndRenderer(GpuTextOverlayResources& res);

}  // namespace sdl3cpp::services::impl
