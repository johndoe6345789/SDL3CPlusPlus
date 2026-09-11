#include "services/interfaces/workflow/gta5/gta5_effects.hpp"

namespace sdl3cpp::services::impl {

void SetUpGta5Effects(Gta5Effects& effects, Gta5StreamState& state,
                      SDL_GPUDevice* device, std::uint32_t maxVertices) {
    CreateGta5EffectAtlas(effects, device, state.uploads);
    const auto bytes =
        static_cast<Uint32>(maxVertices * sizeof(BspRenderVertex));
    SDL_GPUBufferCreateInfo info = {};
    info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    info.size = bytes;
    effects.vertices = SDL_CreateGPUBuffer(device, &info);
    SDL_GPUTransferBufferCreateInfo upload = {};
    upload.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    upload.size = bytes;
    effects.staging = SDL_CreateGPUTransferBuffer(device, &upload);
}

}  // namespace sdl3cpp::services::impl
