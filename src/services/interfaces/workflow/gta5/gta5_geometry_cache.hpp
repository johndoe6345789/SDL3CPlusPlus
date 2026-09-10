#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <SDL3/SDL_gpu.h>

#include <memory>

namespace sdl3cpp::services::impl {

/// Fetch an archetype's GPU mesh, importing and uploading it on first use.
///
/// Returns nullptr when the archetype has no exported model, or failed to
/// import or upload. Failures are cached as unusable.
Gta5Geometry* GetOrLoadGta5Geometry(Gta5StreamState& state,
                                    const Gta5Placement& placement,
                                    SDL_GPUDevice* device,
                                    const std::shared_ptr<ILogger>& logger);

/// Release the GPU buffers of every cached archetype no tile references.
void SweepGta5GeometryCache(Gta5StreamState& state, SDL_GPUDevice* device,
                            const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
