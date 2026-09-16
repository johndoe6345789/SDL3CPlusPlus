#include "services/interfaces/workflow/gta5/resource/gta5_drawable_geometry.hpp"

#include "services/interfaces/workflow/gta5/world/gta5_collision_shape.hpp"
#include "services/interfaces/workflow/gta5/render/gta5_indexed_texture.hpp"
#include "services/interfaces/workflow/gta5/render/gta5_submesh_upload.hpp"
#include "services/interfaces/workflow/gta5/render/gta5_mesh_bounds.hpp"

#include <exception>

namespace sdl3cpp::services::impl {

bool UploadGta5MeshGeometry(Gta5StreamState& state,
                            const Gta5MeshData& mesh, SDL_GPUDevice* device,
                            Gta5Geometry& geometry, bool collide,
                            const std::string& name,
                            const std::shared_ptr<ILogger>& logger) {
    for (const Gta5SubMeshData& part : mesh.parts) {
        Gta5SubMesh sub;
        try {
            // One unuploadable part must not cost the rest of the model.
            if (!UploadGta5SubMesh(part, device, state.arena, state.uploads,
                                   state.textureCache, sub, logger)) {
                continue;
            }
        } catch (const std::exception& ex) {
            if (logger) {
                logger->Warn("gta5: upload failed for '" + name +
                             "': " + ex.what());
            }
            continue;
        }
        if (const Gta5Texture* texture = GetOrLoadGta5IndexedTexture(
                state, part.textureHash, device)) {
            sub.texture     = texture->texture;
            sub.sampler     = texture->sampler;
            sub.textureSize = {texture->width, texture->height};
        }
        // Terrain's layers, or anything else's normal and specular maps.
        for (int i = 0; i < 5; ++i) {
            if (const Gta5Texture* layer = GetOrLoadGta5IndexedTexture(
                    state, part.layerHashes[i], device)) {
                sub.layers[i]        = layer->texture;
                sub.layerSamplers[i] = layer->sampler;
            }
        }
        geometry.subMeshes.push_back(sub);
    }
    if (geometry.subMeshes.empty()) return false;
    if (collide) BuildGta5CollisionShape(mesh, geometry);
    geometry.bounds = Gta5MeshBounds(mesh);
    geometry.usable = true;
    return true;
}

}  // namespace sdl3cpp::services::impl
