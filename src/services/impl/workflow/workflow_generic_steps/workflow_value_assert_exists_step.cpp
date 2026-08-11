#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_assert_exists_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowValueAssertExistsStep::WorkflowValueAssertExistsStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowValueAssertExistsStep::GetPluginId() const {
    return "value.assert.exists";
}

void WorkflowValueAssertExistsStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string inputKey = resolver.GetRequiredInputKey(step, "value");
    if (!context.Contains(inputKey)) {
        throw std::runtime_error("value.assert.exists missing value '" + inputKey + "'");
    }

    if (logger_) {
        logger_->Trace("WorkflowValueAssertExistsStep", "Execute",
                       "key=" + inputKey,
                       "Workflow value exists");
    }
}

}  // namespace sdl3cpp::services::impl
