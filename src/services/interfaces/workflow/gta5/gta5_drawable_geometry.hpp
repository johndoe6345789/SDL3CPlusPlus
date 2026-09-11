#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <SDL3/SDL_gpu.h>

#include <memory>

namespace sdl3cpp::services::impl {

/// Build an archetype's geometry straight from the extracted map.
///
/// Finds its drawable through the asset index, reads it, uploads each
/// part with its texture, and builds the collision mesh from the same
/// triangles. No converted file is involved at any point.
bool BuildGta5DrawableGeometry(Gta5StreamState& state,
                               const Gta5Placement& placement,
                               SDL_GPUDevice* device, Gta5Geometry& geometry,
                               const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
