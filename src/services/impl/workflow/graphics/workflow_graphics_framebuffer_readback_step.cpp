#include "services/interfaces/workflow/graphics/workflow_graphics_framebuffer_readback_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <cstring>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowGraphicsFramebufferReadbackStep::WorkflowGraphicsFramebufferReadbackStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGraphicsFramebufferReadbackStep::GetPluginId() const {
    return "graphics.framebuffer.readback";
}

void WorkflowGraphicsFramebufferReadbackStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;

    // --- Resolve I/O keys ---
    const std::string sourceTextureKeyKey = resolver.GetRequiredInputKey(step, "source_texture_key");
    const std::string outputDataKey       = resolver.GetRequiredOutputKey(step, "output_key");
    const std::string outputWidthKey      = resolver.GetRequiredOutputKey(step, "output_width");
    const std::string outputHeightKey     = resolver.GetRequiredOutputKey(step, "output_height");
    const std::string outputSuccessKey    = resolver.GetRequiredOutputKey(step, "success");

    // --- Read the context key name that holds the source texture ---
    const auto* srcTexKeyPtr = context.TryGet<std::string>(sourceTextureKeyKey);
    const std::string srcTexKey = (srcTexKeyPtr && !srcTexKeyPtr->empty())
                                      ? *srcTexKeyPtr
                                      : std::string("gpu_swapchain_texture");

    // --- Retrieve GPU handles from context ---
    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    SDL_Window*    window = context.Get<SDL_Window*>("sdl_window", nullptr);
    if (!device || !window) {
        throw std::runtime_error(
            "graphics.framebuffer.readback: GPU device or window not found in context");
    }

    // Source texture to read back
    SDL_GPUTexture* source_tex = context.Get<SDL_GPUTexture*>(srcTexKey, nullptr);
    if (!source_tex) {
        throw std::runtime_error(
            "graphics.framebuffer.readback: source texture '" + srcTexKey + "' not found in context");
    }

    // --- Determine dimensions ---
    int win_w = 0, win_h = 0;
    SDL_GetWindowSize(window, &win_w, &win_h);
    if (win_w <= 0 || win_h <= 0) {
        context.Set(outputSuccessKey, false);
        return;
    }
    const uint32_t w = static_cast<uint32_t>(win_w);
    const uint32_t h = static_cast<uint32_t>(win_h);

    // --- Step 1: Acquire command buffer and swapchain for blit source ---
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) {
        throw std::runtime_error(
            "graphics.framebuffer.readback: SDL_AcquireGPUCommandBuffer failed: " +
            std::string(SDL_GetError()));
    }

    // We need a swapchain texture to blit from (GPU-only surface)
    SDL_GPUTexture* swapchain_tex = nullptr;
    Uint32 sw = 0, sh = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchain_tex, &sw, &sh) ||
        !swapchain_tex) {
        SDL_CancelGPUCommandBuffer(cmd);
        context.Set(outputSuccessKey, false);
        return;
    }

    // --- Step 2: Create staging texture for readback ---
    SDL_GPUTextureFormat format = SDL_GetGPUSwapchainTextureFormat(device, window);

    SDL_GPUTextureCreateInfo tex_info = {};
    tex_info.type                     = SDL_GPU_TEXTURETYPE_2D;
    tex_info.format                   = format;
    tex_info.width                    = sw;
    tex_info.height                   = sh;
    tex_info.layer_count_or_depth     = 1;
    tex_info.num_levels               = 1;
    tex_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;

    SDL_GPUTexture* staging_tex = SDL_CreateGPUTexture(device, &tex_info);
    if (!staging_tex) {
        SDL_SubmitGPUCommandBuffer(cmd);
        context.Set(outputSuccessKey, false);
        return;
    }

    // --- Step 3: Blit source to staging ---
    SDL_GPUBlitInfo blit       = {};
    blit.source.texture        = swapchain_tex;
    blit.source.w              = sw;
    blit.source.h              = sh;
    blit.destination.texture   = staging_tex;
    blit.destination.w         = sw;
    blit.destination.h         = sh;
    blit.load_op               = SDL_GPU_LOADOP_DONT_CARE;
    blit.filter                = SDL_GPU_FILTER_LINEAR;

    SDL_BlitGPUTexture(cmd, &blit);
    SDL_SubmitGPUCommandBuffer(cmd);

    // --- Step 4: Create transfer buffer for CPU download ---
    const uint32_t pixel_size = 4;  // RGBA8 / ABGR8888
    const uint32_t row_pitch  = sw * pixel_size;
    const uint32_t total_size = row_pitch * sh;

    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    transfer_info.size  = total_size;

    SDL_GPUTransferBuffer* download_buf = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    if (!download_buf) {
        SDL_ReleaseGPUTexture(device, staging_tex);
        context.Set(outputSuccessKey, false);
        return;
    }

    // --- Step 5: Copy staging texture to transfer buffer ---
    SDL_GPUCommandBuffer* dl_cmd  = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass*      copy_pass = SDL_BeginGPUCopyPass(dl_cmd);

    SDL_GPUTextureTransferInfo dst_transfer = {};
    dst_transfer.transfer_buffer = download_buf;
    dst_transfer.offset          = 0;
    dst_transfer.pixels_per_row  = sw;
    dst_transfer.rows_per_layer  = sh;

    SDL_GPUTextureRegion src_region = {};
    src_region.texture = staging_tex;
    src_region.w       = sw;
    src_region.h       = sh;
    src_region.d       = 1;

    SDL_DownloadFromGPUTexture(copy_pass, &src_region, &dst_transfer);
    SDL_EndGPUCopyPass(copy_pass);

    // --- Step 6: Wait for GPU completion via fence ---
    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(dl_cmd);
    SDL_WaitForGPUFences(device, true, &fence, 1);
    SDL_ReleaseGPUFence(device, fence);

    // --- Step 7: Map transfer buffer and copy pixels to std::vector ---
    void* mapped = SDL_MapGPUTransferBuffer(device, download_buf, false);
    if (!mapped) {
        SDL_ReleaseGPUTransferBuffer(device, download_buf);
        SDL_ReleaseGPUTexture(device, staging_tex);
        context.Set(outputSuccessKey, false);
        return;
    }

    std::vector<uint8_t> pixel_data(total_size);
    std::memcpy(pixel_data.data(), mapped, total_size);
    SDL_UnmapGPUTransferBuffer(device, download_buf);

    // --- Cleanup GPU resources ---
    SDL_ReleaseGPUTransferBuffer(device, download_buf);
    SDL_ReleaseGPUTexture(device, staging_tex);

    // --- Store results in context ---
    context.Set(outputDataKey, std::move(pixel_data));
    context.Set(outputWidthKey, sw);
    context.Set(outputHeightKey, sh);
    context.Set(outputSuccessKey, true);

    if (logger_) {
        logger_->Info("graphics.framebuffer.readback: Read back " +
                      std::to_string(sw) + "x" + std::to_string(sh) +
                      " (" + std::to_string(total_size) + " bytes) into context key '" +
                      outputDataKey + "'");
    }
}

}  // namespace sdl3cpp::services::impl
