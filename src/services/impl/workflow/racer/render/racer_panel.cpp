#include "services/interfaces/workflow/racer/render/racer_panel.hpp"

namespace sdl3cpp::services::impl {

bool CreateRacerTextPanel(SDL_GPUDevice* device, int width, int height,
                          RacerPanel& out) {
    RacerPanel panel;
    panel.device = device;
    panel.width = width;
    panel.height = height;
    SDL_GPUTextureCreateInfo tci = {};
    tci.type = SDL_GPU_TEXTURETYPE_2D;
    tci.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tci.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    tci.width = static_cast<Uint32>(width);
    tci.height = static_cast<Uint32>(height);
    tci.layer_count_or_depth = 1;
    tci.num_levels = 1;
    panel.texture = SDL_CreateGPUTexture(device, &tci);
    SDL_GPUTransferBufferCreateInfo tbci = {};
    tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size = static_cast<Uint32>(width * height * 4);
    panel.transfer = SDL_CreateGPUTransferBuffer(device, &tbci);
    panel.surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32);
    panel.renderer =
        panel.surface ? SDL_CreateSoftwareRenderer(panel.surface) : nullptr;
    if (!panel.texture || !panel.transfer || !panel.renderer) {
        DestroyRacerPanel(panel);
        return false;
    }
    out = panel;
    return true;
}

void DestroyRacerPanel(RacerPanel& panel) {
    if (panel.renderer) SDL_DestroyRenderer(panel.renderer);
    if (panel.surface) SDL_DestroySurface(panel.surface);
    if (panel.device) {
        if (panel.quad) SDL_ReleaseGPUBuffer(panel.device, panel.quad);
        if (panel.transfer) {
            SDL_ReleaseGPUTransferBuffer(panel.device, panel.transfer);
        }
        if (panel.sampler) SDL_ReleaseGPUSampler(panel.device, panel.sampler);
        if (panel.texture) SDL_ReleaseGPUTexture(panel.device, panel.texture);
    }
    panel = RacerPanel{};
}

void DrawRacerPanel(const RacerPanel& panel, SDL_GPUCommandBuffer* cmd,
                    SDL_GPUTexture* swapchain,
                    SDL_GPUGraphicsPipeline* pipeline,
                    SDL_GPUSampler* sampler) {
    SDL_GPUSampler* use = panel.sampler ? panel.sampler : sampler;
    if (!cmd || !swapchain || !pipeline || !use || !panel.quadUploaded) {
        return;
    }
    SDL_GPUColorTargetInfo target = {};
    target.texture = swapchain;
    target.load_op = SDL_GPU_LOADOP_LOAD;
    target.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &target, 1, nullptr);
    if (!pass) return;
    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_GPUBufferBinding vertices = {panel.quad, 0};
    SDL_BindGPUVertexBuffers(pass, 0, &vertices, 1);
    SDL_GPUTextureSamplerBinding texture = {panel.texture, use};
    SDL_BindGPUFragmentSamplers(pass, 0, &texture, 1);
    SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
}

}  // namespace sdl3cpp::services::impl
