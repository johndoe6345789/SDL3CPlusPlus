#include "services/interfaces/workflow/graphics/gpu_framebuffer_readback.hpp"

#include <SDL3/SDL.h>

#include <cstring>

namespace sdl3cpp::services::impl {

BlittedSwapchainStaging BlitSwapchainToStaging(SDL_GPUDevice* device,
                                               SDL_Window* window) {
    BlittedSwapchainStaging out;

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) return out;

    // We need a swapchain texture to blit from (GPU-only surface)
    SDL_GPUTexture* swapchain_tex = nullptr;
    Uint32 sw = 0, sh = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchain_tex,
                                               &sw, &sh) ||
        !swapchain_tex) {
        SDL_CancelGPUCommandBuffer(cmd);
        return out;
    }

    SDL_GPUTextureFormat format =
        SDL_GetGPUSwapchainTextureFormat(device, window);

    SDL_GPUTextureCreateInfo tex_info = {};
    tex_info.type = SDL_GPU_TEXTURETYPE_2D;
    tex_info.format = format;
    tex_info.width = sw;
    tex_info.height = sh;
    tex_info.layer_count_or_depth = 1;
    tex_info.num_levels = 1;
    tex_info.usage =
        SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;

    SDL_GPUTexture* staging_tex = SDL_CreateGPUTexture(device, &tex_info);
    if (!staging_tex) {
        SDL_SubmitGPUCommandBuffer(cmd);
        return out;
    }

    SDL_GPUBlitInfo blit = {};
    blit.source.texture = swapchain_tex;
    blit.source.w = sw;
    blit.source.h = sh;
    blit.destination.texture = staging_tex;
    blit.destination.w = sw;
    blit.destination.h = sh;
    blit.load_op = SDL_GPU_LOADOP_DONT_CARE;
    blit.filter = SDL_GPU_FILTER_LINEAR;

    SDL_BlitGPUTexture(cmd, &blit);
    SDL_SubmitGPUCommandBuffer(cmd);

    out.texture = staging_tex;
    out.width = sw;
    out.height = sh;
    return out;
}

std::vector<uint8_t> DownloadStagingTexture(
    SDL_GPUDevice* device, const BlittedSwapchainStaging& staging) {
    const uint32_t pixel_size = 4;  // RGBA8 / ABGR8888
    const uint32_t row_pitch = staging.width * pixel_size;
    const uint32_t total_size = row_pitch * staging.height;

    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    transfer_info.size = total_size;

    SDL_GPUTransferBuffer* download_buf =
        SDL_CreateGPUTransferBuffer(device, &transfer_info);
    if (!download_buf) {
        SDL_ReleaseGPUTexture(device, staging.texture);
        return {};
    }

    SDL_GPUCommandBuffer* dl_cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(dl_cmd);

    SDL_GPUTextureTransferInfo dst_transfer = {};
    dst_transfer.transfer_buffer = download_buf;
    dst_transfer.offset = 0;
    dst_transfer.pixels_per_row = staging.width;
    dst_transfer.rows_per_layer = staging.height;

    SDL_GPUTextureRegion src_region = {};
    src_region.texture = staging.texture;
    src_region.w = staging.width;
    src_region.h = staging.height;
    src_region.d = 1;

    SDL_DownloadFromGPUTexture(copy_pass, &src_region, &dst_transfer);
    SDL_EndGPUCopyPass(copy_pass);

    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(dl_cmd);
    SDL_WaitForGPUFences(device, true, &fence, 1);
    SDL_ReleaseGPUFence(device, fence);

    void* mapped = SDL_MapGPUTransferBuffer(device, download_buf, false);
    if (!mapped) {
        SDL_ReleaseGPUTransferBuffer(device, download_buf);
        SDL_ReleaseGPUTexture(device, staging.texture);
        return {};
    }

    std::vector<uint8_t> pixel_data(total_size);
    std::memcpy(pixel_data.data(), mapped, total_size);
    SDL_UnmapGPUTransferBuffer(device, download_buf);

    SDL_ReleaseGPUTransferBuffer(device, download_buf);
    SDL_ReleaseGPUTexture(device, staging.texture);
    return pixel_data;
}

}  // namespace sdl3cpp::services::impl
