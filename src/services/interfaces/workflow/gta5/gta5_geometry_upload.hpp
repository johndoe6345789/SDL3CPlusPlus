#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_geometry.hpp"
#include "services/interfaces/workflow/gta5/gta5_placement.hpp"
#include "services/interfaces/workflow/gta5/gta5_texture_cache.hpp"

#include <SDL3/SDL_gpu.h>

#include <memory>

namespace sdl3cpp::services::impl {

/// Import an archetype's model and upload one submesh per material.
///
/// Leaves `geometry` usable only if at least one submesh survived.
/// Returns false, without throwing, on any import or GPU failure.
bool BuildGta5Geometry(const Gta5Placement& placement, SDL_GPUDevice* device,
                       Gta5TextureCache& textures, Gta5Geometry& geometry,
                       const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
