#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: postfx.setup
 *
 * One-time setup for the postfx pipeline: creates the linear/nearest
 * samplers and generates the SSAO hemisphere kernel. See
 * postfx_samplers.hpp for the sampler/kernel construction.
 */
class WorkflowPostfxSetupStep final : public IWorkflowStep {
public:
    explicit WorkflowPostfxSetupStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
