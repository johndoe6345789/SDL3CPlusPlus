#include "services/interfaces/workflow/rendering/workflow_overlay_fps_upload_text_step.hpp"
#include "services/interfaces/workflow/rendering/gpu_text_overlay_resources.hpp"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_timer.h>
#include <cstdio>
#include <cstring>

namespace sdl3cpp::services::impl {

WorkflowOverlayFpsUploadTextStep::WorkflowOverlayFpsUploadTextStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowOverlayFpsUploadTextStep::GetPluginId() const {
    return "overlay.fps_upload_text";
}

void WorkflowOverlayFpsUploadTextStep::Execute(const WorkflowStepDefinition&,
                                               WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) {
        return;
    }

    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* res =
        context.Get<GpuTextOverlayResources*>("overlay_fps_resources", nullptr);
    if (!cmd || !res || !res->surface || !res->renderer || !res->transfer) {
        return;
    }

    const float fps = meter_.Update(SDL_GetTicksNS());

    char text[32];
    std::snprintf(text, sizeof(text), "FPS: %.0f", fps);

    SDL_SetRenderDrawColor(res->renderer, 0, 0, 0, 0);
    SDL_RenderClear(res->renderer);
    SDL_SetRenderDrawColor(res->renderer, 255, 220, 50, 255);
    SDL_RenderDebugText(res->renderer, 5.0f, 2.0f, text);
    SDL_RenderPresent(res->renderer);  // flush batch to surface->pixels

    void* mapped = SDL_MapGPUTransferBuffer(res->device, res->transfer, false);
    if (!mapped) {
        return;
    }
    std::memcpy(
        mapped, res->surface->pixels,
        static_cast<size_t>(kGpuTextOverlayWidth * kGpuTextOverlayHeight * 4));
    SDL_UnmapGPUTransferBuffer(res->device, res->transfer);

    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    if (!copy) {
        return;
    }

    SDL_GPUTextureTransferInfo src = {};
    src.transfer_buffer            = res->transfer;
    src.pixels_per_row             = static_cast<Uint32>(kGpuTextOverlayWidth);
    src.rows_per_layer             = static_cast<Uint32>(kGpuTextOverlayHeight);

    SDL_GPUTextureRegion dst = {};
    dst.texture              = res->texture;
    dst.w                    = static_cast<Uint32>(kGpuTextOverlayWidth);
    dst.h                    = static_cast<Uint32>(kGpuTextOverlayHeight);
    dst.d                    = 1;

    SDL_UploadToGPUTexture(copy, &src, &dst, false);
    SDL_EndGPUCopyPass(copy);
}

}  // namespace sdl3cpp::services::impl
