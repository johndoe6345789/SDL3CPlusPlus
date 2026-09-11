#include "services/interfaces/workflow/gta5/gta5_submesh_upload.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

static_assert(sizeof(BspRenderVertex) == Gta5GeometryArena::kVertexStride,
              "the arena is laid out for position_uv_lmuv_normal");

bool UploadGta5SubMesh(const Gta5SubMeshData& part, SDL_GPUDevice* device,
                       Gta5GeometryArena& arena, Gta5UploadBatch& uploads,
                       Gta5TextureCache& textures, Gta5SubMesh& out,
                       const std::shared_ptr<ILogger>& logger) {
    if (part.vertices.empty() || part.indices.empty()) return false;
    if (part.vertices.size() >= kGta5MaxVerticesPerMesh) return false;

    if (!arena.Upload(device, uploads, part.vertices.data(),
                      static_cast<std::uint32_t>(part.vertices.size()),
                      part.indices.data(),
                      static_cast<std::uint32_t>(part.indices.size()),
                      out.slot)) {
        throw std::runtime_error(std::string("geometry arena upload: ") +
                                 SDL_GetError());
    }
    out.indexCount = static_cast<std::uint32_t>(part.indices.size());
    out.paint = part.paint;
    out.surface = {part.tint[0], part.tint[1], part.tint[2],
                   part.alphaCutoff};
    out.blend = part.blend;
    out.terrain = part.terrain;
    out.emissive = part.emissive && !part.blend;  // glass stays glass
    // Terrain reads its surface alpha as "has a lookup mask" -- it has no
    // cutout to use it for.
    if (part.terrain) out.surface[3] = part.layerHashes[4] ? 1.f : 0.f;

    // A part with no texture still draws; the shader falls back to the
    // map's default, which is better than dropping the geometry.
    const Gta5Texture* texture =
        GetOrLoadGta5Texture(textures, part.texturePath, device, logger);
    if (texture) {
        out.texture = texture->texture;
        out.sampler = texture->sampler;
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
