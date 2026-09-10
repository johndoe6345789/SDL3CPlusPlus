#include "services/interfaces/workflow/quake3/q3_sky_scroll.hpp"

#include <cmath>
#include <cstring>

namespace sdl3cpp::services::impl {
namespace {

/// textures/skies/tim_hell's cloud stages: `tcMod scroll 0.05 .1` then
/// `tcMod scale 2 2`. Shader tcMods are not parsed yet, so the only sky
/// shipped with the maps we load is spelled out here.
constexpr float kScrollS = 0.05f;
constexpr float kScrollT = 0.10f;
constexpr float kScale   = 2.0f;

float WrappedScroll(float speed, float elapsed) {
    const float offset = speed * elapsed;
    return offset - std::floor(offset);
}

}  // namespace

void UpdateSkyScroll(SDL_GPUDevice* device, SkyResources& sky, float elapsed) {
    if (!device || !sky.transfer || sky.baseVertices.empty()) {
        return;
    }

    const float offsetS = WrappedScroll(kScrollS, elapsed);
    const float offsetT = WrappedScroll(kScrollT, elapsed);

    sky.scratch = sky.baseVertices;
    for (auto& vertex : sky.scratch) {
        vertex.u = (vertex.u + offsetS) * kScale;
        vertex.v = (vertex.v + offsetT) * kScale;
    }

    const auto bytes =
        static_cast<uint32_t>(sky.scratch.size() * sizeof(BspRenderVertex));
    void* mapped = SDL_MapGPUTransferBuffer(device, sky.transfer, true);
    if (!mapped) {
        return;
    }
    std::memcpy(mapped, sky.scratch.data(), bytes);
    SDL_UnmapGPUTransferBuffer(device, sky.transfer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) {
        return;
    }
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);
    if (copyPass) {
        SDL_GPUTransferBufferLocation src = {};
        src.transfer_buffer               = sky.transfer;
        SDL_GPUBufferRegion dst           = {};
        dst.buffer                        = sky.vertexBuffer;
        dst.size                          = bytes;
        // Cycle: last frame's draw may still be reading the buffer.
        SDL_UploadToGPUBuffer(copyPass, &src, &dst, true);
        SDL_EndGPUCopyPass(copyPass);
    }
    SDL_SubmitGPUCommandBuffer(cmd);
}

}  // namespace sdl3cpp::services::impl
