#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: bsp.lightmap_atlas
 *
 * Packs the loaded BSP's LUMP_LIGHTMAPS blocks into one GPU texture atlas
 * (see bsp_lightmap_atlas.hpp), and stores the atlas texture/sampler plus
 * its grid layout in context for the face-building steps to compute UVs
 * from.
 *
 * Reads from context: bsp_raw_data, gpu_device.
 * Writes to context: bsp_lightmap_atlas_gpu, bsp_lightmap_atlas_sampler,
 *   bsp_grid_size, bsp_num_lightmaps.
 */
class WorkflowBspLightmapAtlasStep final : public IWorkflowStep {
public:
    explicit WorkflowBspLightmapAtlasStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
