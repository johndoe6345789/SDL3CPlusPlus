#pragma once

#include "services/interfaces/workflow/gta5/gta5_map_travel.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_map_overlay.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.map.draw
 *
 * Tab opens a full-screen map -- GTA's own pause-map art -- with an arrow
 * where the player is, pointing the way the camera looks; Tab again
 * closes it. Runs after the composite. Parameters: minimap_dir, the
 * extract's data/cdimages/scaleform_generic.rpf; and the world rectangle
 * the art covers, map_min_x, map_max_y (GTA's north edge), map_width and
 * map_height, in metres; poi_file, the points of interest (JSON).
 */
class WorkflowGta5MapDrawStep final : public IWorkflowStep {
public:
    WorkflowGta5MapDrawStep(std::shared_ptr<ILogger> logger,
                            std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    Gta5MapOverlay map_;
    Gta5MapTravel travel_;  // double-click to go there
    bool open_{false};
    bool held_{false};
};

}  // namespace sdl3cpp::services::impl
