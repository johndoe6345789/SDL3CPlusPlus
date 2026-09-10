#include "services/interfaces/workflow/rendering/workflow_overlay_sw_end_upload_quad_step.hpp"
#include "services/interfaces/workflow/rendering/overlay_sw_end_resources.hpp"

#include <cstring>

namespace sdl3cpp::services::impl {

WorkflowOverlaySwEndUploadQuadStep::WorkflowOverlaySwEndUploadQuadStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowOverlaySwEndUploadQuadStep::GetPluginId() const {
    return "overlay.sw.end_upload_quad";
}

void WorkflowOverlaySwEndUploadQuadStep::Execute(const WorkflowStepDefinition&,
                                                 WorkflowContext& context) {
    if (context.GetBool("frame_skip", false) ||
        !context.GetBool("overlay.ready", false)) {
        return;
    }

    auto* res = context.Get<OverlaySwEndResources*>("overlay_sw_end_resources",
                                                    nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    if (!res || res->vertexBufferUploaded || !cmd) {
        return;
    }

    static constexpr float kVerts[6][5] = {
        {-1, 1, 0, 0, 0}, {1, 1, 0, 1, 0},  {1, -1, 0, 1, 1},
        {-1, 1, 0, 0, 0}, {1, -1, 0, 1, 1}, {-1, -1, 0, 0, 1},
    };
    const uint32_t size = sizeof(kVerts);

    SDL_GPUTransferBufferCreateInfo tbci = {};
    tbci.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size                            = size;
    SDL_GPUTransferBuffer* staging =
        SDL_CreateGPUTransferBuffer(res->device, &tbci);
    if (!staging) {
        return;
    }
    if (void* mapped = SDL_MapGPUTransferBuffer(res->device, staging, false)) {
        std::memcpy(mapped, kVerts, size);
        SDL_UnmapGPUTransferBuffer(res->device, staging);
    }
    if (SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd)) {
        SDL_GPUTransferBufferLocation src = {staging, 0};
        SDL_GPUBufferRegion dst           = {res->vertices, 0, size};
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);
    }
    SDL_ReleaseGPUTransferBuffer(res->device, staging);
    res->vertexBufferUploaded = true;
}

}  // namespace sdl3cpp::services::impl
