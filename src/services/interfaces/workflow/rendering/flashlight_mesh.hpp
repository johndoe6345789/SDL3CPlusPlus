#pragma once

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Vertex format: float3 pos + float2 uv = 20 bytes (matches the
/// plane/textured pipeline).
struct PosUvVertex {
    float x, y, z;
    float u, v;
};

/// A built flashlight mesh plus the world-space Y where its lens cap sits
/// (the barrel runs along +Y), so a caller can place a spotlight there.
struct FlashlightMesh {
    std::vector<PosUvVertex> vertices;
    std::vector<uint16_t> indices;
    float lensY = 0.0f;
};

/**
 * @brief Builds a flashlight mesh: grip cylinder, head cylinder, lens cap.
 *
 * The Y axis is the barrel direction. `segments` controls the radial
 * tessellation of every cylinder/cap.
 */
FlashlightMesh BuildFlashlightMesh(int segments, float bodyRadius,
                                   float bodyLength, float headRadius,
                                   float headLength, float lensRadius);

/// Uploaded GPU vertex/index buffers for a FlashlightMesh.
struct FlashlightMeshBuffers {
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer = nullptr;
};

/// Creates GPU vertex/index buffers and uploads `mesh` into them via a
/// staging transfer buffer, submitting its own command buffer.
FlashlightMeshBuffers UploadFlashlightMesh(SDL_GPUDevice* device,
                                           const FlashlightMesh& mesh);

}  // namespace sdl3cpp::services::impl
