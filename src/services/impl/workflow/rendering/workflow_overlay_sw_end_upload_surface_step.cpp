#include "services/interfaces/workflow/rendering/workflow_overlay_sw_end_upload_surface_step.hpp"
#include "services/interfaces/workflow/rendering/overlay_sw_end_resources.hpp"

#include <SDL3/SDL_surface.h>
#include <cstring>

namespace sdl3cpp::services::impl {

WorkflowOverlaySwEndUploadSurfaceStep::WorkflowOverlaySwEndUploadSurfaceStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowOverlaySwEndUploadSurfaceStep::GetPluginId() const {
    return "overlay.sw.end_upload_surface";
}

void WorkflowOverlaySwEndUploadSurfaceStep::Execute(
    const WorkflowStepDefinition&, WorkflowContext& context) {
    if (context.GetBool("frame_skip", false) ||
        !context.GetBool("overlay.ready", false)) {
        return;
    }

    auto* surface = context.Get<SDL_Surface*>("overlay.surface", nullptr);
    auto* device  = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* res = context.Get<OverlaySwEndResources*>("overlay_sw_end_resources",
                                                    nullptr);
    if (!surface || !device || !res) {
        return;
    }

    void* mapped = SDL_MapGPUTransferBuffer(device, res->transfer, false);
    if (!mapped) {
        return;
    }
    std::memcpy(mapped, surface->pixels,
                static_cast<size_t>(surface->w) * surface->h * 4);
    SDL_UnmapGPUTransferBuffer(device, res->transfer);

    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    if (!cmd) {
        return;
    }
    if (SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd)) {
        SDL_GPUTextureTransferInfo src = {};
        src.transfer_buffer            = res->transfer;
        src.pixels_per_row             = static_cast<uint32_t>(surface->w);
        src.rows_per_layer             = static_cast<uint32_t>(surface->h);

        SDL_GPUTextureRegion dst = {};
        dst.texture              = res->texture;
        dst.w                    = static_cast<uint32_t>(surface->w);
        dst.h                    = static_cast<uint32_t>(surface->h);
        dst.d                    = 1;

        SDL_UploadToGPUTexture(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);
    }
}

}  // namespace sdl3cpp::services::impl
