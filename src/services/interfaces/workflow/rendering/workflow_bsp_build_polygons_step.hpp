#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: bsp.build_polygons
 *
 * Walks the BSP's polygon and mesh faces (types 1 and 3) into the shared
 * per-texture group map, fixing Q3's clockwise winding to the engine's
 * counter-clockwise convention.  Must run after `bsp.tessellate_patches`,
 * which owns the group map, and before `bsp.flatten_geometry`.
 *
 * Reads from context: bsp_raw_data, bsp_config, bsp_grid_size,
 *   bsp_num_lightmaps, bsp_texture_groups.
 */
class WorkflowBspBuildPolygonsStep final : public IWorkflowStep {
public:
    explicit WorkflowBspBuildPolygonsStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
