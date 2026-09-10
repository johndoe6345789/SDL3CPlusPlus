#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_format_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/string_template.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowStringFormatStep::WorkflowStringFormatStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowStringFormatStep::GetPluginId() const {
    return "string.format";
}

void WorkflowStringFormatStep::Execute(const WorkflowStepDefinition& step,
                                       WorkflowContext& context) {
    const auto templateIt = step.inputs.find("template");
    if (templateIt == step.inputs.end()) {
        throw std::runtime_error("string.format requires 'template' input");
    }
    const std::string& templateStr = templateIt->second;

    // Get template content from context if it's a key
    std::string template_content;
    if (const auto* templateValue =
            context.TryGet<std::string>(templateStr)) {
        template_content = *templateValue;
    } else {
        template_content = templateStr;
    }

    // Values map key is optional (a map of variable names to context keys)
    const auto valuesIt = step.inputs.find("values");
    const std::string valuesKey =
        (valuesIt != step.inputs.end()) ? valuesIt->second : std::string();

    const auto outputIt = step.inputs.find("output");
    if (outputIt == step.inputs.end()) {
        throw std::runtime_error("string.format requires 'output' input");
    }
    const std::string& outputKey = outputIt->second;

    const std::string formatted =
        InterpolateTemplate(context, template_content, valuesKey);
    context.Set(outputKey, formatted);

    if (logger_) {
        logger_->Trace(
            "WorkflowStringFormatStep", "Execute",
            "template_length=" +
                std::to_string(template_content.length()) +
                ", result=" + formatted,
            "String formatted successfully");
    }
}

}  // namespace sdl3cpp::services::impl
