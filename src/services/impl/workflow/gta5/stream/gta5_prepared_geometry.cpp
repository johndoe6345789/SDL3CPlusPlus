#include "services/interfaces/workflow/gta5/stream/gta5_prepared_geometry.hpp"

#include "services/interfaces/workflow/gta5/world/gta5_collision_shape.hpp"
#include "services/interfaces/workflow/gta5/resource/gta5_drawable_geometry.hpp"
#include "services/interfaces/workflow/gta5/resource/gta5_drawable_mesh.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

/// A texture from the drawable's own dictionary when it embeds one --
/// many props and trees carry theirs inside the .ydr, where no .ytd index
/// finds them, and drew as the package's grey default -- else from the
/// .ytd that holds it.
Gta5TextureBlob ReadTexture(const Gta5AssetIndex& index,
                            Gta5ResourceCache& resources,
                            const Gta5IndexedDrawable& owner,
                            std::uint32_t hash) {
    const std::int64_t group = owner.res->Follow(owner.drawable + 0x10);
    const std::int64_t embedded =
        group < 0 ? -1 : owner.res->Follow(group + 0x08);
    Gta5TextureBlob blob =
        ReadGta5DictionaryTexture(*owner.res, hash, embedded);
    if (!blob.bytes.empty()) return blob;
    const auto where = index.textures.find(hash);
    const auto ytd = where == index.textures.end()
                         ? nullptr
                         : AcquireGta5Resource(resources, index, where->second);
    return ytd ? ReadGta5DictionaryTexture(*ytd, hash) : blob;
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
    const Gta5IndexedDrawable owner =
        AcquireGta5IndexedDrawable(index, resources, hash);
    if (!owner.res) return out;
    out.mesh = ReadGta5DrawableMesh(*owner.res, owner.drawable);
    if (out.mesh.parts.empty()) return out;

    // Collision here, not on the main thread: building a BVH is the
    // costliest thing done to a mesh.
    BuildGta5CollisionShape(out.mesh, out.collision);
    for (const Gta5SubMeshData& part : out.mesh.parts) {
        const std::uint32_t wanted[] = {
            part.textureHash, part.layerHashes[0], part.layerHashes[1],
            part.layerHashes[2], part.layerHashes[3], part.layerHashes[4]};
        for (const std::uint32_t texture : wanted) {
            if (texture == 0 ||
                std::find(out.textures.begin(), out.textures.end(),
                          texture) != out.textures.end()) {
                continue;
            }
            out.textures.push_back(texture);
            if (claims.Claim(texture)) {
                out.blobs.push_back(
                    ReadTexture(index, resources, owner, texture));
            }
        }
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
