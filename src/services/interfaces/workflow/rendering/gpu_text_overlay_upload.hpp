#pragma once

#include "services/interfaces/workflow/rendering/gpu_text_overlay_resources.hpp"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_pixels.h>

namespace sdl3cpp::services::impl {

/// Draw `text` into the overlay's surface with SDL's debug font and copy
/// it to the overlay texture. An empty string clears it, and a cleared
/// overlay draws as nothing, which is how a label is taken down without
/// a visibility switch in the draw step. False if the copy could not be
/// set up.
bool UploadGpuTextOverlayText(const GpuTextOverlayResources& res,
                              SDL_GPUCommandBuffer* cmd, const char* text,
                              SDL_Color colour);

}  // namespace sdl3cpp::services::impl
