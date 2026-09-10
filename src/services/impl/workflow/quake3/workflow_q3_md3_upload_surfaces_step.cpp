#include "services/interfaces/workflow/quake3/workflow_q3_md3_upload_surfaces_step.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_format.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_source.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_surface_uploader.hpp"

#include <SDL3/SDL_gpu.h>

#include <string>

namespace sdl3cpp::services::impl {

WorkflowQ3Md3UploadSurfacesStep::WorkflowQ3Md3UploadSurfacesStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3Md3UploadSurfacesStep::GetPluginId() const {
    return "q3.md3.upload_surfaces";
}

void WorkflowQ3Md3UploadSurfacesStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    const Q3Md3StepParameters params = ReadQ3Md3StepParameters(step);
    auto* source =
        context.Get<Q3Md3Source*>(Q3Md3SourceKey(params.prefix), nullptr);
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!source || !device || source->bytes.size() < sizeof(q3::Md3Header)) {
        return;
    }

    const uint8_t* base  = source->bytes.data();
    const auto& header   = *reinterpret_cast<const q3::Md3Header*>(base);
    const int frameCount = header.numFrames;
    if (frameCount <= 0 || header.numSurfaces <= 0) {
        return;
    }

    const uint8_t* surfacePtr = base + header.ofsSurfaces;
    for (int s = 0; s < header.numSurfaces; ++s) {
        const auto& surface =
            *reinterpret_cast<const q3::Md3Surface*>(surfacePtr);
        const std::string keyPrefix =
            "q3.md3." + params.prefix + "_surf" + std::to_string(s);

        UploadMd3Surface(device, *source, surfacePtr, surface, frameCount,
                         keyPrefix, context);

        surfacePtr += surface.ofsEnd;
    }

    if (logger_) {
        logger_->Info("q3.md3.upload_surfaces[" + params.prefix + "]: loaded " +
                      source->path + " (" + std::to_string(header.numSurfaces) +
                      " surfs, " + std::to_string(frameCount) + " frames)");
    }
}

}  // namespace sdl3cpp::services::impl
