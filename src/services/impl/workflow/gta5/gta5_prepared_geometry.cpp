#include "services/interfaces/workflow/gta5/gta5_prepared_geometry.hpp"

#include "services/interfaces/workflow/gta5/gta5_collision_shape.hpp"
#include "services/interfaces/workflow/gta5/gta5_drawable_geometry.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

Gta5TextureBlob ReadTexture(const Gta5AssetIndex& index,
                            Gta5ResourceCache& resources,
                            std::uint32_t hash) {
    const auto where = index.textures.find(hash);
    const auto ytd = where == index.textures.end()
                         ? nullptr
                         : AcquireGta5Resource(resources, index, where->second);
    if (!ytd) {
        Gta5TextureBlob missing;
        missing.hash = hash;
        return missing;
    }
    return ReadGta5DictionaryTexture(*ytd, hash);
}

}  // namespace

bool Gta5TextureClaims::Claim(std::uint32_t hash) {
    std::lock_guard<std::mutex> hold(lock_);
    return claimed_.insert(hash).second;
}

Gta5PreparedGeometry PrepareGta5Geometry(const Gta5AssetIndex& index,
                                         Gta5ResourceCache& resources,
                                         Gta5TextureClaims& claims,
                                         const std::string& key,
                                         std::uint32_t hash) {
    Gta5PreparedGeometry out;
    out.key = key;
    out.mesh = ReadGta5IndexedMesh(index, resources, hash);
    if (out.mesh.parts.empty()) return out;

    // Collision here, not on the main thread: building a BVH is the
    // costliest thing done to a mesh.
    BuildGta5CollisionShape(out.mesh, out.collision);
    for (const Gta5SubMeshData& part : out.mesh.parts) {
        const std::uint32_t texture = part.textureHash;
        if (texture == 0 || std::find(out.textures.begin(), out.textures.end(),
                                      texture) != out.textures.end()) {
            continue;
        }
        out.textures.push_back(texture);
        if (claims.Claim(texture)) {
            out.blobs.push_back(ReadTexture(index, resources, texture));
        }
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
