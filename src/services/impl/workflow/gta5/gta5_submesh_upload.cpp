#include "services/interfaces/workflow/gta5/gta5_submesh_upload.hpp"

#include "services/interfaces/workflow/graphics/graphics_gpu_buffer_upload.hpp"

#include <cstring>
#include <vector>

namespace sdl3cpp::services::impl {

bool UploadGta5SubMesh(const Gta5SubMeshData& part, SDL_GPUDevice* device,
                       Gta5TextureCache& textures, Gta5SubMesh& out,
                       const std::shared_ptr<ILogger>& logger) {
    if (part.vertices.empty() || part.indices.empty()) return false;
    if (part.vertices.size() >= kGta5MaxVerticesPerMesh) return false;

    std::vector<uint8_t> bytes(part.vertices.size() * sizeof(BspRenderVertex));
    std::memcpy(bytes.data(), part.vertices.data(), bytes.size());
    const UploadedGpuBuffers buffers =
        CreateAndUploadGpuBuffers(device, bytes, part.indices);

    out.vertexBuffer = buffers.vertexBuffer;
    out.indexBuffer = buffers.indexBuffer;
    out.indexCount = static_cast<std::uint32_t>(part.indices.size());
    out.surface = {part.tint[0], part.tint[1], part.tint[2],
                   part.alphaCutoff};

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
