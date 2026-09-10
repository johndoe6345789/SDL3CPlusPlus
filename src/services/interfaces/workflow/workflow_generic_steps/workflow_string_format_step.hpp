#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: string.format
 *
 * Expands `{name}` placeholders in the "template" input (a literal string
 * or a context key holding one) against context values, writing the
 * result to the "output" context key. See string_template.hpp for the
 * interpolation logic.
 */
class WorkflowStringFormatStep final : public IWorkflowStep {
public:
    explicit WorkflowStringFormatStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
