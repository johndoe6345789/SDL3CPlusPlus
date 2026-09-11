#include "services/interfaces/workflow/gta5/gta5_drawable_geometry.hpp"

#include "services/interfaces/workflow/gta5/gta5_collision_shape.hpp"
#include "services/interfaces/workflow/gta5/gta5_indexed_texture.hpp"
#include "services/interfaces/workflow/gta5/gta5_submesh_upload.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <exception>

namespace sdl3cpp::services::impl {
namespace {

/// A sphere round the box's centre, reaching the furthest vertex.
std::array<float, 4> MeshBounds(const Gta5MeshData& mesh) {
    glm::vec3 lo(1e30f), hi(-1e30f);
    for (const Gta5SubMeshData& part : mesh.parts) {
        for (const BspRenderVertex& v : part.vertices) {
            lo = glm::min(lo, glm::vec3(v.x, v.y, v.z));
            hi = glm::max(hi, glm::vec3(v.x, v.y, v.z));
        }
    }
    if (lo.x > hi.x) return {0.f, 0.f, 0.f, -1.f};
    const glm::vec3 c = (lo + hi) * 0.5f;
    float r2 = 0.f;
    for (const Gta5SubMeshData& part : mesh.parts) {
        for (const BspRenderVertex& v : part.vertices) {
            const glm::vec3 d = glm::vec3(v.x, v.y, v.z) - c;
            r2 = std::max(r2, glm::dot(d, d));
        }
    }
    return {c.x, c.y, c.z, std::sqrt(r2)};
}

}  // namespace

bool UploadGta5MeshGeometry(Gta5StreamState& state, const Gta5MeshData& mesh,
                            SDL_GPUDevice* device, Gta5Geometry& geometry,
                            bool collide, const std::string& name,
                            const std::shared_ptr<ILogger>& logger) {
    for (const Gta5SubMeshData& part : mesh.parts) {
        Gta5SubMesh sub;
        try {
            // One unuploadable part must not cost the rest of the model.
            if (!UploadGta5SubMesh(part, device, state.arena,
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
            sub.texture = texture->texture;
            sub.sampler = texture->sampler;
        }
        for (int i = 0; part.terrain && i < 4; ++i) {
            if (const Gta5Texture* layer = GetOrLoadGta5IndexedTexture(
                    state, part.layerHashes[i], device)) {
                sub.layers[i] = layer->texture;
                sub.layerSamplers[i] = layer->sampler;
            }
        }
        geometry.subMeshes.push_back(sub);
    }
    if (geometry.subMeshes.empty()) return false;
    if (collide) BuildGta5CollisionShape(mesh, geometry);
    geometry.bounds = MeshBounds(mesh);
    geometry.usable = true;
    return true;
}

}  // namespace sdl3cpp::services::impl
