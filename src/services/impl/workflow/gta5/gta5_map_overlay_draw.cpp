#include "services/interfaces/workflow/gta5/gta5_map_overlay.hpp"

#include <cstring>

namespace sdl3cpp::services::impl {

void DrawGta5MapOverlay(Gta5MapOverlay& map, SDL_GPUDevice* device,
                        SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* swapchain,
                        const std::vector<float>& quads) {
    const auto bytes = static_cast<Uint32>(quads.size() * sizeof(float));
    void* mapped = SDL_MapGPUTransferBuffer(device, map.staging, true);
    if (!mapped) return;
    std::memcpy(mapped, quads.data(), bytes);
    SDL_UnmapGPUTransferBuffer(device, map.staging);
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    const SDL_GPUTransferBufferLocation from = {map.staging, 0};
    const SDL_GPUBufferRegion to = {map.vertices, 0, bytes};
    SDL_UploadToGPUBuffer(copy, &from, &to, true);
    SDL_EndGPUCopyPass(copy);

    SDL_GPUColorTargetInfo target = {};
    target.texture = swapchain;
    target.load_op = SDL_GPU_LOADOP_LOAD;  // over the finished frame
    target.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &target, 1, nullptr);
    if (!pass) return;
    SDL_BindGPUGraphicsPipeline(pass, map.pipeline);
    const SDL_GPUBufferBinding vertices = {map.vertices, 0};
    SDL_BindGPUVertexBuffers(pass, 0, &vertices, 1);
    const auto draw = [&](SDL_GPUTexture* texture, int quad) {
        const SDL_GPUTextureSamplerBinding binding = {texture, map.sampler};
        SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
        SDL_DrawGPUPrimitives(pass, 6, 1, static_cast<Uint32>(quad * 6), 0);
    };
    draw(map.shade, 0);
    for (int i = 0; i < 6; ++i) draw(map.tiles[i], 1 + i);
    draw(map.marker, 7);
    SDL_EndGPURenderPass(pass);
}

}  // namespace sdl3cpp::services::impl
