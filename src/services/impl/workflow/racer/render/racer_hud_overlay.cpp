#include "services/interfaces/workflow/racer/render/racer_hud_overlay.hpp"

namespace sdl3cpp::services::impl {

bool CreateRacerHudOverlay(SDL_GPUDevice* device, RacerHudOverlay& out) {
    RacerHudOverlay hud;
    hud.device = device;
    SDL_GPUTextureCreateInfo tci = {};
    tci.type = SDL_GPU_TEXTURETYPE_2D;
    tci.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tci.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    tci.width = kRacerHudWidth;
    tci.height = kRacerHudHeight;
    tci.layer_count_or_depth = 1;
    tci.num_levels = 1;
    hud.texture = SDL_CreateGPUTexture(device, &tci);
    SDL_GPUTransferBufferCreateInfo tbci = {};
    tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size = kRacerHudWidth * kRacerHudHeight * 4 + 6 * 5 * 4;
    hud.transfer = SDL_CreateGPUTransferBuffer(device, &tbci);
    SDL_GPUBufferCreateInfo bci = {};
    bci.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bci.size = 6 * 5 * sizeof(float);
    hud.quad = SDL_CreateGPUBuffer(device, &bci);
    hud.surface = SDL_CreateSurface(kRacerHudWidth, kRacerHudHeight,
                                    SDL_PIXELFORMAT_RGBA32);
    hud.renderer = hud.surface ? SDL_CreateSoftwareRenderer(hud.surface)
                               : nullptr;
    const bool ok = hud.texture && hud.transfer && hud.quad && hud.renderer;
    if (!ok) {
        DestroyRacerHudOverlay(hud);
        return false;
    }
    out = hud;
    return true;
}

void DestroyRacerHudOverlay(RacerHudOverlay& hud) {
    if (hud.renderer) SDL_DestroyRenderer(hud.renderer);
    if (hud.surface) SDL_DestroySurface(hud.surface);
    if (hud.device) {
        if (hud.quad) SDL_ReleaseGPUBuffer(hud.device, hud.quad);
        if (hud.transfer) {
            SDL_ReleaseGPUTransferBuffer(hud.device, hud.transfer);
        }
        if (hud.texture) SDL_ReleaseGPUTexture(hud.device, hud.texture);
    }
    hud = RacerHudOverlay{};
}

void DrawRacerHud(const RacerHudOverlay& hud, SDL_GPUCommandBuffer* cmd,
                  SDL_GPUTexture* swapchain,
                  SDL_GPUGraphicsPipeline* pipeline, SDL_GPUSampler* sampler) {
    if (!cmd || !swapchain || !pipeline || !sampler || !hud.quadUploaded) {
        return;
    }
    SDL_GPUColorTargetInfo target = {};
    target.texture = swapchain;
    target.load_op = SDL_GPU_LOADOP_LOAD;
    target.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &target, 1, nullptr);
    if (!pass) return;
    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_GPUBufferBinding vertices = {hud.quad, 0};
    SDL_BindGPUVertexBuffers(pass, 0, &vertices, 1);
    SDL_GPUTextureSamplerBinding texture = {hud.texture, sampler};
    SDL_BindGPUFragmentSamplers(pass, 0, &texture, 1);
    SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
}

}  // namespace sdl3cpp::services::impl
