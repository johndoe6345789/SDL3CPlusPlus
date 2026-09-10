#pragma once

#include "services/interfaces/workflow/quake3/q3_md3_format.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <string>

namespace sdl3cpp::services::impl {

/// Uploads `{keyPrefix}_ib` (index buffer) and `_num_idx` (index count)
/// for one MD3 surface's already-CCW triangle indices.
void UploadIndexBuffer(SDL_GPUDevice* device, const uint8_t* surfacePtr,
                       const q3::Md3Surface& surface,
                       const std::string& keyPrefix, WorkflowContext& context);

/// Uploads `{keyPrefix}_f{f}_vb` for each of `frameCount` animation frames,
/// decoding each frame's vertices into engine space as it goes.
void UploadVertexBuffers(SDL_GPUDevice* device, const uint8_t* surfacePtr,
                         const q3::Md3Surface& surface, int frameCount,
                         const std::string& keyPrefix,
                         WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
