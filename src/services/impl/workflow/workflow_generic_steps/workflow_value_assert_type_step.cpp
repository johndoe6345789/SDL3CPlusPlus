#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_assert_type_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowValueAssertTypeStep::WorkflowValueAssertTypeStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowValueAssertTypeStep::GetPluginId() const {
    return "value.assert.type";
}

void WorkflowValueAssertTypeStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver ioResolver;
    WorkflowStepParameterResolver parameterResolver;
    const std::string inputKey = ioResolver.GetRequiredInputKey(step, "value");
    std::string type = parameterResolver.GetRequiredString(step, "type");
    std::transform(type.begin(), type.end(), type.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    if (!context.Contains(inputKey)) {
        throw std::runtime_error("value.assert.type missing value '" + inputKey + "'");
    }

    if (type == "any") {
        return;
    }

    const bool matches = (type == "string" && context.TryGet<std::string>(inputKey)) ||
        (type == "number" && context.TryGet<double>(inputKey)) ||
        (type == "bool" && context.TryGet<bool>(inputKey)) ||
        (type == "string_list" && context.TryGet<std::vector<std::string>>(inputKey)) ||
        (type == "number_list" && context.TryGet<std::vector<double>>(inputKey)) ||
        (type == "path" && context.TryGet<std::filesystem::path>(inputKey));

    if (!matches) {
        throw std::runtime_error("value.assert.type mismatch for '" + inputKey + "', expected " + type);
    }

    if (logger_) {
        logger_->Trace("WorkflowValueAssertTypeStep", "Execute",
                       "key=" + inputKey + ", type=" + type,
                       "Workflow value type confirmed");
    }
}

}  // namespace sdl3cpp::services::impl
