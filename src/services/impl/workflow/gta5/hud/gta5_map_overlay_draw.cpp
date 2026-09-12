#include "services/interfaces/workflow/gta5/hud/gta5_map_build.hpp"

#include <algorithm>
#include <cstring>

namespace sdl3cpp::services::impl {

void DrawGta5MapOverlay(Gta5MapOverlay& map, SDL_GPUDevice* device,
                        SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* swapchain,
                        const Gta5MapFrame& frame) {
    const std::size_t floats = std::min<std::size_t>(
        frame.vertices.size(), std::size_t{kGta5MapMaxQuads} * 30);
    if (floats == 0) return;
    const auto bytes = static_cast<Uint32>(floats * sizeof(float));
    void* mapped = SDL_MapGPUTransferBuffer(device, map.staging, true);
    if (!mapped) return;
    std::memcpy(mapped, frame.vertices.data(), bytes);
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
    for (const Gta5MapRange& range : frame.ranges) {
        const SDL_GPUTextureSamplerBinding binding = {range.texture,
                                                      range.sampler};
        SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
        SDL_DrawGPUPrimitives(pass, range.count * 6, 1, range.first * 6, 0);
    }
    SDL_EndGPURenderPass(pass);
}

}  // namespace sdl3cpp::services::impl
