#include "services/interfaces/workflow/workflow_generic_steps/workflow_array_create_step.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowArrayCreateStep::WorkflowArrayCreateStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
}

std::string WorkflowArrayCreateStep::GetPluginId() const {
    return "array.create";
}

void WorkflowArrayCreateStep::Execute(const WorkflowStepDefinition& step,
                                      WorkflowContext& context) {
    // Get output key
    const auto outputIt = step.inputs.find("output");
    if (outputIt == step.inputs.end()) {
        throw std::runtime_error("array.create requires 'output' input");
    }
    const std::string& outputKey = outputIt->second;

    // Create empty vector stored as std::any
    std::vector<std::any> emptyArray;
    context.Set(outputKey, emptyArray);

    if (logger_) {
        logger_->Trace("WorkflowArrayCreateStep", "Execute", "output=" + outputKey,
                       "Empty array created");
    }
}

}  // namespace sdl3cpp::services::impl
