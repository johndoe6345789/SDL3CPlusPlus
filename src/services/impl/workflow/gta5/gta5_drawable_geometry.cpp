#include "services/interfaces/workflow/gta5/gta5_drawable_geometry.hpp"

#include "services/interfaces/workflow/gta5/gta5_drawable_mesh.hpp"
#include "services/interfaces/workflow/gta5/gta5_resource_cache.hpp"

namespace sdl3cpp::services::impl {

Gta5MeshData ReadGta5ArchetypeMesh(Gta5StreamState& state,
                                   std::uint32_t hash,
                                   const glm::vec3& paint) {
    if (!state.assets) return {};
    const Gta5AssetIndex& index = *state.assets;
    const auto where = index.drawables.find(hash);
    if (where == index.drawables.end()) return {};
    const auto res = AcquireGta5Resource(state.resources, index, where->second);
    if (!res) return {};
    const std::int64_t drawable =
        LocateGta5Drawable(*res, index.files[where->second], hash);
    return drawable < 0 ? Gta5MeshData{}
                        : ReadGta5DrawableMesh(*res, drawable, paint);
}

bool BuildGta5DrawableGeometry(Gta5StreamState& state,
                               const Gta5Placement& placement,
                               SDL_GPUDevice* device, Gta5Geometry& geometry,
                               const std::shared_ptr<ILogger>& logger) {
    return UploadGta5MeshGeometry(
        state, ReadGta5ArchetypeMesh(state, placement.archetypeHash), device,
        geometry, true, placement.archetype, logger);
}

}  // namespace sdl3cpp::services::impl
