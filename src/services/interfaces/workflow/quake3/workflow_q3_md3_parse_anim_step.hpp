#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.md3.parse_anim
 *
 * Parses the animation.cfg named by the step's `anim` parameter into
 * q3.md3.{prefix}_anim.  A model without an `anim` parameter is skipped.
 */
class WorkflowQ3Md3ParseAnimStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3Md3ParseAnimStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
