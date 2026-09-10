#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: bsp.load
 *
 * Opens the "pk3_path" zip, lists every map it contains into
 * context["q3.maps"], reads and validates "map_name" (or the
 * "q3.pending_map" context key, if set — takes precedence for in-process
 * map switching), and stores the raw BSP bytes and load config.
 *
 * Writes to context: q3.maps, bsp_raw_data, bsp_config.
 */
class WorkflowBspLoadStep final : public IWorkflowStep {
public:
    explicit WorkflowBspLoadStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
