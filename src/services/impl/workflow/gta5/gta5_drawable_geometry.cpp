#include "services/interfaces/workflow/gta5/gta5_drawable_geometry.hpp"

#include "services/interfaces/workflow/gta5/gta5_collision_shape.hpp"
#include "services/interfaces/workflow/gta5/gta5_drawable_mesh.hpp"
#include "services/interfaces/workflow/gta5/gta5_indexed_texture.hpp"
#include "services/interfaces/workflow/gta5/gta5_resource_cache.hpp"
#include "services/interfaces/workflow/gta5/gta5_submesh_upload.hpp"

#include <exception>

namespace sdl3cpp::services::impl {
namespace {

Gta5MeshData ReadMesh(Gta5StreamState& state, std::uint32_t hash) {
    const Gta5AssetIndex& index = *state.assets;
    const auto where = index.drawables.find(hash);
    if (where == index.drawables.end()) return {};
    const auto res = AcquireGta5Resource(state.resources, index, where->second);
    if (!res) return {};
    const std::int64_t drawable =
        LocateGta5Drawable(*res, index.files[where->second], hash);
    return drawable < 0 ? Gta5MeshData{} : ReadGta5DrawableMesh(*res, drawable);
}

}  // namespace

bool BuildGta5DrawableGeometry(Gta5StreamState& state,
                               const Gta5Placement& placement,
                               SDL_GPUDevice* device, Gta5Geometry& geometry,
                               const std::shared_ptr<ILogger>& logger) {
    if (!state.assets) return false;
    const Gta5MeshData mesh = ReadMesh(state, placement.archetypeHash);

    for (const Gta5SubMeshData& part : mesh.parts) {
        Gta5SubMesh sub;
        try {
            // One unuploadable part must not cost the rest of the building.
            if (!UploadGta5SubMesh(part, device, state.textureCache, sub,
                                   logger)) {
                continue;
            }
        } catch (const std::exception& ex) {
            if (logger) {
                logger->Warn("gta5.tiles.load: upload failed for '" +
                             placement.archetype + "': " + ex.what());
            }
            continue;
        }
        if (const Gta5Texture* texture = GetOrLoadGta5IndexedTexture(
                state, part.textureHash, device)) {
            sub.texture = texture->texture;
            sub.sampler = texture->sampler;
        }
        geometry.subMeshes.push_back(sub);
    }
    if (geometry.subMeshes.empty()) return false;

    // Collide against the drawn triangles: stand on what is visible.
    BuildGta5CollisionShape(mesh, geometry);
    geometry.usable = true;
    return true;
}

}  // namespace sdl3cpp::services::impl
