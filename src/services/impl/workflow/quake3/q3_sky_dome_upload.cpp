#include "services/interfaces/workflow/quake3/q3_sky_dome_upload.hpp"

#include "services/interfaces/workflow/graphics/graphics_gpu_buffer_upload.hpp"
#include "services/interfaces/workflow/quake3/q3_sky_dome.hpp"

#include <cstring>
#include <exception>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

/// Comfortably outside any q3 map's extents but inside the camera's
/// 100-unit far plane, so the dome is never clipped away.
constexpr float kRadius = 70.0f;
constexpr int kSegments = 32;
constexpr int kRings    = 12;

}  // namespace

bool UploadSkyDome(SDL_GPUDevice* device, SkyResources& out) {
    const SkyDomeMesh mesh = BuildSkyDome(kRadius, kSegments, kRings);
    if (mesh.vertices.empty() || mesh.indices.empty()) {
        return false;
    }

    std::vector<uint8_t> vertexBytes(mesh.vertices.size() *
                                     sizeof(BspRenderVertex));
    std::memcpy(vertexBytes.data(), mesh.vertices.data(), vertexBytes.size());

    UploadedGpuBuffers buffers;
    try {
        buffers = CreateAndUploadGpuBuffers(device, vertexBytes, mesh.indices);
    } catch (const std::exception&) {
        return false;
    }

    // The uvs are rewritten every frame to scroll the clouds, so keep one
    // transfer buffer rather than making a new one per frame.
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size  = static_cast<uint32_t>(vertexBytes.size());

    out.transfer     = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    out.baseVertices = mesh.vertices;
    out.vertexBuffer = buffers.vertexBuffer;
    out.indexBuffer  = buffers.indexBuffer;
    out.indexCount   = static_cast<uint32_t>(mesh.indices.size());
    return true;
}

}  // namespace sdl3cpp::services::impl
