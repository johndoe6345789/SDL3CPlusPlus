#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: bsp.flatten_geometry
 *
 * Flattens the per-texture group map built by `bsp.tessellate_patches` and
 * `bsp.build_polygons` into one vertex buffer and one index buffer, with a
 * `map.nodes` entry per texture group giving that group's index range.
 *
 * Reads from context: bsp_raw_data, bsp_config, bsp_texture_groups.
 * Writes to context:
 *   bsp_all_vertices  shared_ptr<vector<BspRenderVertex>>
 *   bsp_all_indices   shared_ptr<vector<uint32_t>>
 *   bsp_used_textures shared_ptr<set<int>>
 *   map.nodes         nlohmann::json (appended to by later map-loading steps)
 *
 * @throws std::runtime_error if no texture group produced any geometry.
 */
class WorkflowBspFlattenGeometryStep final : public IWorkflowStep {
public:
    explicit WorkflowBspFlattenGeometryStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
