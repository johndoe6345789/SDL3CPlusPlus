#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

// If the boolean context key named by input "condition" is true,
// write the JSON parameter "value" to the context key named by output "value".
// Plugin ID: value.set_if
class WorkflowValueSetIfStep final : public IWorkflowStep {
public:
    explicit WorkflowValueSetIfStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
