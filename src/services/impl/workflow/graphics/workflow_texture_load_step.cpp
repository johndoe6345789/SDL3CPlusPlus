#include "services/interfaces/workflow/graphics/workflow_texture_load_step.hpp"
#include "services/interfaces/workflow/graphics/texture_load_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowTextureLoadStep::WorkflowTextureLoadStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowTextureLoadStep::GetPluginId() const {
    return "texture.load";
}

void WorkflowTextureLoadStep::Execute(const WorkflowStepDefinition& step,
                                      WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string pathKey =
        resolver.GetRequiredInputKey(step, "image_path");
    const std::string outputKey =
        resolver.GetRequiredOutputKey(step, "texture");

    const auto* image_path = context.TryGet<std::string>(pathKey);
    if (!image_path) {
        throw std::runtime_error(
            "texture.load: image_path not found in context key '" + pathKey +
            "'");
    }

    const std::string resolved = ResolveTextureImagePath(*image_path);
    if (logger_) {
        logger_->Trace("WorkflowTextureLoadStep", "Execute",
                       "path=" + resolved, "Loading texture");
    }

    LoadedTextureImage image = LoadTextureImagePixels(resolved);

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        FreeTextureImagePixels(image);
        throw std::runtime_error(
            "texture.load: GPU device not found in context");
    }

    const UploadedTexture uploaded = UploadTextureImage(device, image);
    SDL_GPUSampler* sampler =
        CreateTextureLoadSampler(device, uploaded.texture, uploaded.numLevels);

    context.Set<SDL_GPUTexture*>(outputKey + "_gpu", uploaded.texture);
    context.Set<SDL_GPUSampler*>(outputKey + "_sampler", sampler);

    const Uint32 data_size =
        static_cast<Uint32>(image.width * image.height * 4);
    nlohmann::json meta = {{"valid", true},
                            {"width", image.width},
                            {"height", image.height},
                            {"channels", 4},
                            {"path", resolved}};
    context.Set(outputKey, meta);

    if (logger_) {
        logger_->Info("texture.load: Loaded " + resolved + " (" +
                     std::to_string(image.width) + "x" +
                     std::to_string(image.height) + ", " +
                     std::to_string(data_size) + " bytes)");
    }
}

}  // namespace sdl3cpp::services::impl
