#include "services/interfaces/workflow/rendering/workflow_overlay_fps_upload_quad_step.hpp"
#include "services/interfaces/workflow/rendering/gpu_text_overlay_resources.hpp"
#include "services/interfaces/workflow/rendering/gpu_overlay_quad.hpp"

#include <SDL3/SDL_gpu.h>
#include <cstring>

namespace sdl3cpp::services::impl {

WorkflowOverlayFpsUploadQuadStep::WorkflowOverlayFpsUploadQuadStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowOverlayFpsUploadQuadStep::GetPluginId() const {
    return "overlay.fps_upload_quad";
}

void WorkflowOverlayFpsUploadQuadStep::Execute(const WorkflowStepDefinition&,
                                               WorkflowContext& context) {
    if (uploaded_ || context.GetBool("frame_skip", false)) {
        return;
    }

    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* res =
        context.Get<GpuTextOverlayResources*>("overlay_fps_resources", nullptr);
    if (!cmd || !res || !res->vertices || !res->device) {
        return;
    }

    const GpuOverlayQuad quad = BuildGpuOverlayQuad(
        static_cast<int>(context.Get<uint32_t>("frame_width", 1280u)),
        static_cast<int>(context.Get<uint32_t>("frame_height", 960u)),
        kGpuTextOverlayWidth, kGpuTextOverlayHeight);
    const Uint32 size = static_cast<Uint32>(quad.size() * sizeof(float));

    SDL_GPUTransferBufferCreateInfo tbci = {};
    tbci.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size                            = size;
    SDL_GPUTransferBuffer* staging =
        SDL_CreateGPUTransferBuffer(res->device, &tbci);
    if (!staging) {
        return;
    }

    if (void* mapped = SDL_MapGPUTransferBuffer(res->device, staging, false)) {
        std::memcpy(mapped, quad.data(), size);
        SDL_UnmapGPUTransferBuffer(res->device, staging);
    }

    if (SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd)) {
        SDL_GPUTransferBufferLocation src = {staging, 0};
        SDL_GPUBufferRegion dst           = {res->vertices, 0, size};
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);
    }

    SDL_ReleaseGPUTransferBuffer(res->device, staging);
    uploaded_ = true;

    if (logger_) {
        logger_->Trace("WorkflowOverlayFpsUploadQuadStep", "Execute",
                       "bytes=" + std::to_string(size),
                       "Uploaded the static FPS overlay quad");
    }
}

}  // namespace sdl3cpp::services::impl
