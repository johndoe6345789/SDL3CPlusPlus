#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/render/gta5_mesh_extract.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// An archetype's drawable: the file holding it, opened, and where in
/// that file the drawable starts. res is null when there is none.
struct Gta5IndexedDrawable {
    std::shared_ptr<const Gta5Resource> res;
    std::int64_t drawable{-1};
};

/// Find and open an archetype's drawable through the index and resource
/// cache alone, touching no stream state, so the load pool's workers
/// call it.
Gta5IndexedDrawable AcquireGta5IndexedDrawable(const Gta5AssetIndex& index,
                                               Gta5ResourceCache& resources,
                                               std::uint32_t hash);

/// Its mesh; `paint` tints vehicle_paint geometry. Empty when there is
/// no such drawable.
Gta5MeshData ReadGta5IndexedMesh(const Gta5AssetIndex& index,
                                 Gta5ResourceCache& resources,
                                 std::uint32_t hash,
                                 const glm::vec3& paint = glm::vec3(1.f));

/// The same, on the main thread, through the stream state's index.
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

/// Both, synchronously, for a placement: its archetype's geometry
/// straight from the extracted map.
bool BuildGta5DrawableGeometry(Gta5StreamState& state,
                               const Gta5Placement& placement,
                               SDL_GPUDevice* device, Gta5Geometry& geometry,
                               const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
