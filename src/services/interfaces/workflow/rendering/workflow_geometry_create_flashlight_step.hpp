#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: geometry.create_flashlight
 *
 * Builds a flashlight mesh (grip cylinder, head cylinder, lens cap) and
 * uploads it to GPU vertex/index buffers under "plane_<name>_vb"/"_ib",
 * with mesh metadata (including the lens Y for spotlight placement) under
 * "plane_<name>".
 */
class WorkflowGeometryCreateFlashlightStep final : public IWorkflowStep {
public:
    explicit WorkflowGeometryCreateFlashlightStep(
        std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
