#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.mapselect
 *
 * Draws the "CHOOSE LEVEL" map-select screen when q3.menu_screen ==
 * "map_select". See q3_mapselect_draw.hpp for the drawing logic; this
 * step only gathers context state into a Q3MapSelectAssets and early-
 * returns when the overlay/menu aren't in the right state to draw.
 */
class WorkflowQ3MapSelectStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3MapSelectStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
