#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: bsp.tessellate_patches
 *
 * Walks the BSP's Bezier patch faces (type 2) and tessellates each into
 * triangles, grouped by texture.  Must run before `bsp.build_polygons` and
 * `bsp.flatten_geometry`: it creates the shared per-texture group map that
 * those steps read and add to.
 *
 * Reads from context: bsp_raw_data, bsp_config, bsp_grid_size,
 *   bsp_num_lightmaps, and the step's `patch_tess_level` parameter (default 4).
 * Writes to context: bsp_texture_groups -> shared_ptr<map<int, TextureGroup>>
 */
class WorkflowBspTessellatePatchesStep final : public IWorkflowStep {
public:
    explicit WorkflowBspTessellatePatchesStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
