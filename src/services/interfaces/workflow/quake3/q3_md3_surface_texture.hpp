#pragma once

#include "services/interfaces/workflow/quake3/q3_md3_format.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_source.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <string>

namespace sdl3cpp::services::impl {

/// Resolves and uploads `{keyPrefix}_tex`/`_samp` for one MD3 surface:
/// tries the skin map, the surface's own shader name, and the model's
/// skin/path in turn, falling back to a 1x1 grey texture if none load.
void UploadSurfaceTexture(SDL_GPUDevice* device, const Q3Md3Source& source,
                          const uint8_t* surfacePtr,
                          const q3::Md3Surface& surface,
                          const std::string& keyPrefix,
                          WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
