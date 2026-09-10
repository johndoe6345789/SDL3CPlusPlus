#include "services/interfaces/workflow/rendering/workflow_bsp_lightmap_atlas_step.hpp"
#include "services/interfaces/workflow/rendering/bsp_lightmap_atlas.hpp"

#include <SDL3/SDL_gpu.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowBspLightmapAtlasStep::WorkflowBspLightmapAtlasStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBspLightmapAtlasStep::GetPluginId() const {
    return "bsp.lightmap_atlas";
}

void WorkflowBspLightmapAtlasStep::Execute(const WorkflowStepDefinition&,
                                           WorkflowContext& context) {
    auto bspDataPtr = context.Get<std::shared_ptr<std::vector<uint8_t>>>(
        "bsp_raw_data", nullptr);
    if (!bspDataPtr) {
        throw std::runtime_error(
            "bsp.lightmap_atlas: bsp_raw_data not in context");
    }

    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error("bsp.lightmap_atlas: GPU device not found");
    }

    const LightmapAtlas atlas  = BuildLightmapAtlas(*bspDataPtr);
    const LightmapAtlasGpu gpu = UploadLightmapAtlas(device, atlas);

    context.Set<SDL_GPUTexture*>("bsp_lightmap_atlas_gpu", gpu.texture);
    context.Set<SDL_GPUSampler*>("bsp_lightmap_atlas_sampler", gpu.sampler);

    // Store grid info for downstream steps
    context.Set("bsp_grid_size", atlas.gridSize);
    context.Set("bsp_num_lightmaps", atlas.numLightmaps);

    if (logger_) {
        logger_->Info("bsp.lightmap_atlas: " + std::to_string(atlas.width) +
                      "x" + std::to_string(atlas.height) + " (" +
                      std::to_string(atlas.numLightmaps) + " lightmaps, grid " +
                      std::to_string(atlas.gridSize) + "x" +
                      std::to_string(atlas.gridSize) + ")");
    }
}

}  // namespace sdl3cpp::services::impl
