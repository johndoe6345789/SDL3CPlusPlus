#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_geometry.hpp"
#include "services/interfaces/workflow/gta5/gta5_mesh_extract.hpp"
#include "services/interfaces/workflow/gta5/gta5_texture_cache.hpp"

#include <SDL3/SDL_gpu.h>

#include <memory>

namespace sdl3cpp::services::impl {

/// Upload one material into the geometry arena and resolve its texture.
///
/// Returns false for a part too large for 16-bit indices. Throws if the
/// GPU upload fails, which the caller catches per part so one bad
/// material does not cost the rest of the building.
bool UploadGta5SubMesh(const Gta5SubMeshData& part, SDL_GPUDevice* device,
                       Gta5GeometryArena& arena, Gta5TextureCache& textures,
                       Gta5SubMesh& out,
                       const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
