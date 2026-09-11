#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <SDL3/SDL_gpu.h>

#include <memory>

namespace sdl3cpp::services::impl {

/// Fetch an archetype's GPU mesh.
///
/// A map archetype is prepared by the load pool: nullptr comes back with
/// `pending` set until it lands, and the caller should come back for it.
/// A legacy tile file's archetype is imported from its glTF here and now.
/// Either way nullptr without `pending` means nothing to draw, cached so
/// it is not tried again.
Gta5Geometry* GetOrLoadGta5Geometry(Gta5StreamState& state,
                                    const Gta5Placement& placement,
                                    SDL_GPUDevice* device,
                                    const std::shared_ptr<ILogger>& logger,
                                    bool* pending = nullptr);

/// Release the GPU buffers of every cached archetype no tile references.
void SweepGta5GeometryCache(Gta5StreamState& state, SDL_GPUDevice* device,
                            const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
