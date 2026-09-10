#pragma once

#include "services/interfaces/workflow/quake3/q3_md3_format.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_source.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>

namespace sdl3cpp::services::impl {

/**
 * @brief Uploads one MD3 surface's geometry and texture to the GPU.
 *
 * Writes `{keyPrefix}_ib`, `_num_idx`, `_f{f}_vb` (one per frame), `_tex` and
 * `_samp` into the context.  A no-op if the surface has no vertices or
 * triangles (a valid, if unusual, MD3 surface).
 *
 * @param surfacePtr Pointer to this surface's Md3Surface header within the
 *                    model's raw bytes (offsets inside it are relative to it).
 */
void UploadMd3Surface(SDL_GPUDevice* device, const Q3Md3Source& source,
                      const uint8_t* surfacePtr, const q3::Md3Surface& surface,
                      int frameCount, const std::string& keyPrefix,
                      WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
