#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// An archetype's mesh, read from whichever file the asset index says
/// holds it; `paint` tints vehicle_paint geometry. Empty when there is
/// no such drawable, or it cannot be read.
Gta5MeshData ReadGta5ArchetypeMesh(Gta5StreamState& state,
                                   std::uint32_t hash,
                                   const glm::vec3& paint = glm::vec3(1.f));

/// Upload a mesh read from the map into `geometry`, textures resolved
/// through the index. With `collide`, the collision mesh is built from
/// the same triangles -- stand on what is visible. `name` is for logs.
bool UploadGta5MeshGeometry(Gta5StreamState& state, const Gta5MeshData& mesh,
                            SDL_GPUDevice* device, Gta5Geometry& geometry,
                            bool collide, const std::string& name,
                            const std::shared_ptr<ILogger>& logger);

/// Both, for a placement: its archetype's geometry straight from the
/// extracted map. No converted file is involved at any point.
bool BuildGta5DrawableGeometry(Gta5StreamState& state,
                               const Gta5Placement& placement,
                               SDL_GPUDevice* device, Gta5Geometry& geometry,
                               const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
