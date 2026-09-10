#include "services/interfaces/workflow/rendering/flashlight_mesh.hpp"

#include <cmath>
#include <cstring>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPi = 3.14159265358979f;

/// Adds a cylinder section (or cone frustum, when r1 != r2) as a ring of
/// vertices at each end and two triangles per segment between them.
void AddCylinder(std::vector<PosUvVertex>& vertices,
                 std::vector<uint16_t>& indices, int segments, float r1,
                 float r2, float y_start, float y_end, float uv_start,
                 float uv_end) {
    uint16_t base = static_cast<uint16_t>(vertices.size());

    for (int i = 0; i <= segments; ++i) {
        float angle = (static_cast<float>(i) / segments) * 2.0f * kPi;
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);
        float u     = static_cast<float>(i) / segments;

        vertices.push_back({cos_a * r1, y_start, sin_a * r1, u, uv_start});
        vertices.push_back({cos_a * r2, y_end, sin_a * r2, u, uv_end});
    }

    for (int i = 0; i < segments; ++i) {
        uint16_t b = base + static_cast<uint16_t>(i * 2);
        indices.push_back(b);
        indices.push_back(b + 1);
        indices.push_back(b + 2);
        indices.push_back(b + 2);
        indices.push_back(b + 1);
        indices.push_back(b + 3);
    }
}

/// Adds a disc cap (a center vertex fanned out to a ring). `flip` reverses
/// winding order for a cap that faces -Y instead of +Y.
void AddCap(std::vector<PosUvVertex>& vertices, std::vector<uint16_t>& indices,
            int segments, float radius, float y, float uv_v, bool flip) {
    uint16_t center = static_cast<uint16_t>(vertices.size());
    vertices.push_back({0.0f, y, 0.0f, 0.5f, uv_v});

    for (int i = 0; i <= segments; ++i) {
        float angle = (static_cast<float>(i) / segments) * 2.0f * kPi;
        vertices.push_back({std::cos(angle) * radius, y,
                            std::sin(angle) * radius,
                            0.5f + 0.5f * std::cos(angle), uv_v});
    }

    for (int i = 0; i < segments; ++i) {
        uint16_t a = center + 1 + static_cast<uint16_t>(i);
        uint16_t b = a + 1;
        if (flip) {
            indices.push_back(center);
            indices.push_back(b);
            indices.push_back(a);
        } else {
            indices.push_back(center);
            indices.push_back(a);
            indices.push_back(b);
        }
    }
}

}  // namespace

FlashlightMesh BuildFlashlightMesh(int segments, float bodyRadius,
                                   float bodyLength, float headRadius,
                                   float headLength, float lensRadius) {
    FlashlightMesh mesh;
    auto& v   = mesh.vertices;
    auto& idx = mesh.indices;

    // Body: long cylinder (handle/grip)
    AddCap(v, idx, segments, bodyRadius, 0.0f, 0.0f, true);  // bottom cap
    AddCylinder(v, idx, segments, bodyRadius, bodyRadius, 0.0f, bodyLength,
                0.0f, 0.6f);

    // Head: slightly wider cylinder (where the bulb sits)
    AddCylinder(v, idx, segments, bodyRadius, headRadius, bodyLength,
                bodyLength + 0.02f, 0.6f, 0.7f);
    AddCylinder(v, idx, segments, headRadius, headRadius, bodyLength + 0.02f,
                bodyLength + headLength, 0.7f, 0.9f);

    // Lens: flat disc at the front (the light-emitting surface)
    mesh.lensY = bodyLength + headLength;
    AddCap(v, idx, segments, lensRadius, mesh.lensY, 1.0f, false);

    return mesh;
}

FlashlightMeshBuffers UploadFlashlightMesh(SDL_GPUDevice* device,
                                           const FlashlightMesh& mesh) {
    const uint32_t vertex_size =
        static_cast<uint32_t>(mesh.vertices.size() * sizeof(PosUvVertex));
    const uint32_t index_size =
        static_cast<uint32_t>(mesh.indices.size() * sizeof(uint16_t));

    SDL_GPUBufferCreateInfo vbuf_info = {};
    vbuf_info.usage                   = SDL_GPU_BUFFERUSAGE_VERTEX;
    vbuf_info.size                    = vertex_size;
    SDL_GPUBuffer* vertex_buffer      = SDL_CreateGPUBuffer(device, &vbuf_info);

    SDL_GPUBufferCreateInfo ibuf_info = {};
    ibuf_info.usage                   = SDL_GPU_BUFFERUSAGE_INDEX;
    ibuf_info.size                    = index_size;
    SDL_GPUBuffer* index_buffer       = SDL_CreateGPUBuffer(device, &ibuf_info);

    SDL_GPUTransferBufferCreateInfo tbuf_info = {};
    tbuf_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbuf_info.size  = vertex_size + index_size;
    SDL_GPUTransferBuffer* transfer =
        SDL_CreateGPUTransferBuffer(device, &tbuf_info);

    auto* mapped = static_cast<uint8_t*>(
        SDL_MapGPUTransferBuffer(device, transfer, false));
    std::memcpy(mapped, mesh.vertices.data(), vertex_size);
    std::memcpy(mapped + vertex_size, mesh.indices.data(), index_size);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* cmd  = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation src_vert = {};
    src_vert.transfer_buffer               = transfer;
    SDL_GPUBufferRegion dst_vert           = {};
    dst_vert.buffer                        = vertex_buffer;
    dst_vert.size                          = vertex_size;
    SDL_UploadToGPUBuffer(copy_pass, &src_vert, &dst_vert, false);

    SDL_GPUTransferBufferLocation src_idx = {};
    src_idx.transfer_buffer               = transfer;
    src_idx.offset                        = vertex_size;
    SDL_GPUBufferRegion dst_idx           = {};
    dst_idx.buffer                        = index_buffer;
    dst_idx.size                          = index_size;
    SDL_UploadToGPUBuffer(copy_pass, &src_idx, &dst_idx, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    return {vertex_buffer, index_buffer};
}

}  // namespace sdl3cpp::services::impl
