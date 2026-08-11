#include "services/interfaces/workflow/graphics/workflow_graphics_screenshot_request_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <cstdlib>
#include <vector>

// Use SDL_SaveBMP for screenshot output (no extra dependencies)

namespace sdl3cpp::services::impl {

WorkflowGraphicsScreenshotRequestStep::WorkflowGraphicsScreenshotRequestStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGraphicsScreenshotRequestStep::GetPluginId() const {
    return "graphics.screenshot.request";
}

void WorkflowGraphicsScreenshotRequestStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string outputPathKey = resolver.GetRequiredInputKey(step, "output_path");
    const std::string outputSuccessKey = resolver.GetRequiredOutputKey(step, "success");

    const auto* output_path = context.TryGet<std::string>(outputPathKey);
    if (!output_path || output_path->empty()) {
        // No path set yet — skip silently (e.g. waiting for frame threshold)
        context.Set(outputSuccessKey, false);
        return;
    }

    // Resolve ~ in path
    std::string resolved_path = *output_path;
    if (!resolved_path.empty() && resolved_path[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home) {
            resolved_path = std::string(home) + resolved_path.substr(1);
        }
    }

    // Create output directory
    std::filesystem::path path(resolved_path);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }

    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    SDL_Window* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    if (!device || !window) {
        throw std::runtime_error("graphics.screenshot.request: GPU device or window not found");
    }

    // Get window dimensions
    int w = 0, h = 0;
    SDL_GetWindowSize(window, &w, &h);
    if (w <= 0 || h <= 0) {
        context.Set(outputSuccessKey, false);
        return;
    }

    // We need to render one more frame to a texture we can read back
    // Acquire a new command buffer for the screenshot
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);

    // Get swapchain texture
    SDL_GPUTexture* swapchain_tex = nullptr;
    Uint32 sw = 0, sh = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchain_tex, &sw, &sh) || !swapchain_tex) {
        SDL_CancelGPUCommandBuffer(cmd);
        context.Set(outputSuccessKey, false);
        return;
    }

    // Create a staging texture we can download from
    SDL_GPUTextureFormat format = SDL_GetGPUSwapchainTextureFormat(device, window);

    SDL_GPUTextureCreateInfo tex_info = {};
    tex_info.type = SDL_GPU_TEXTURETYPE_2D;
    tex_info.format = format;
    tex_info.width = sw;
    tex_info.height = sh;
    tex_info.layer_count_or_depth = 1;
    tex_info.num_levels = 1;
    tex_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;

    SDL_GPUTexture* staging_tex = SDL_CreateGPUTexture(device, &tex_info);
    if (!staging_tex) {
        SDL_SubmitGPUCommandBuffer(cmd);
        context.Set(outputSuccessKey, false);
        return;
    }

    // Blit swapchain to staging texture
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

    // Now download from staging texture
    uint32_t pixel_size = 4;  // RGBA8
    uint32_t row_pitch = sw * pixel_size;
    uint32_t total_size = row_pitch * sh;

    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    transfer_info.size = total_size;

    SDL_GPUTransferBuffer* download_buf = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    if (!download_buf) {
        SDL_ReleaseGPUTexture(device, staging_tex);
        context.Set(outputSuccessKey, false);
        return;
    }

    // Copy texture to transfer buffer
    SDL_GPUCommandBuffer* dl_cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(dl_cmd);

    SDL_GPUTextureTransferInfo dst_transfer = {};
    dst_transfer.transfer_buffer = download_buf;
    dst_transfer.offset = 0;
    dst_transfer.pixels_per_row = sw;
    dst_transfer.rows_per_layer = sh;

    SDL_GPUTextureRegion src_region = {};
    src_region.texture = staging_tex;
    src_region.w = sw;
    src_region.h = sh;
    src_region.d = 1;

    SDL_DownloadFromGPUTexture(copy_pass, &src_region, &dst_transfer);
    SDL_EndGPUCopyPass(copy_pass);

    // Use fence to wait for download
    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(dl_cmd);
    SDL_WaitForGPUFences(device, true, &fence, 1);
    SDL_ReleaseGPUFence(device, fence);

    // Map and save using SDL_Surface → SDL_SaveBMP
    void* mapped = SDL_MapGPUTransferBuffer(device, download_buf, false);
    if (mapped) {
        // Create SDL surface from pixel data
        SDL_Surface* surface = SDL_CreateSurfaceFrom(
            static_cast<int>(sw), static_cast<int>(sh),
            SDL_PIXELFORMAT_ABGR8888,
            mapped, static_cast<int>(row_pitch));

        if (surface) {
            // Save as BMP (change extension if needed)
            std::string save_path = resolved_path;
            // Replace .png with .bmp if present
            if (save_path.size() > 4 && save_path.substr(save_path.size() - 4) == ".png") {
                save_path = save_path.substr(0, save_path.size() - 4) + ".bmp";
            }
            SDL_SaveBMP(surface, save_path.c_str());
            SDL_DestroySurface(surface);

            if (logger_) {
                logger_->Info("graphics.screenshot.request: Saved " +
                              std::to_string(sw) + "x" + std::to_string(sh) +
                              " screenshot to " + save_path);
            }
            context.Set(outputSuccessKey, true);
        } else {
            context.Set(outputSuccessKey, false);
        }

        SDL_UnmapGPUTransferBuffer(device, download_buf);
    } else {
        context.Set(outputSuccessKey, false);
    }

    // Cleanup
    SDL_ReleaseGPUTransferBuffer(device, download_buf);
    SDL_ReleaseGPUTexture(device, staging_tex);
}

}  // namespace sdl3cpp::services::impl
