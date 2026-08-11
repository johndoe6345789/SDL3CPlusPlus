#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_format_step.hpp"

#include <stdexcept>
#include <utility>
#include <sstream>
#include <regex>

namespace sdl3cpp::services::impl {

// Helper to convert any value to string
static std::string AnyToString(const std::any& value) {
    if (auto* s = std::any_cast<std::string>(&value)) {
        return *s;
    }
    if (auto* d = std::any_cast<double>(&value)) {
        return std::to_string(*d);
    }
    if (auto* i = std::any_cast<int>(&value)) {
        return std::to_string(*i);
    }
    if (auto* b = std::any_cast<bool>(&value)) {
        return *b ? "true" : "false";
    }
    return "";
}

WorkflowStringFormatStep::WorkflowStringFormatStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
}

std::string WorkflowStringFormatStep::GetPluginId() const {
    return "string.format";
}

void WorkflowStringFormatStep::Execute(const WorkflowStepDefinition& step,
                                       WorkflowContext& context) {
    // Get template string
    const auto templateIt = step.inputs.find("template");
    if (templateIt == step.inputs.end()) {
        throw std::runtime_error("string.format requires 'template' input");
    }
    const std::string& templateStr = templateIt->second;

    // Get template content from context if it's a key
    std::string template_content;
    const auto* templateValue = context.TryGet<std::string>(templateStr);
    if (templateValue) {
        template_content = *templateValue;
    } else {
        template_content = templateStr;
    }

    // Get values map key (optional - contains map of variable names to context keys)
    const auto valuesIt = step.inputs.find("values");

    // Get output key
    const auto outputIt = step.inputs.find("output");
    if (outputIt == step.inputs.end()) {
        throw std::runtime_error("string.format requires 'output' input");
    }
    const std::string& outputKey = outputIt->second;

    // Perform string interpolation
    std::string result = template_content;
    std::regex placeholder_regex(R"(\{([^}]+)\})");
    std::smatch match;
    std::string::const_iterator search_start(result.cbegin());

    std::string formatted;
    while (std::regex_search(search_start, result.cend(), match, placeholder_regex)) {
        // Add text before placeholder
        formatted.append(match.prefix().first, match.prefix().second);

        // Get placeholder name
        std::string placeholder_name = match[1].str();

        // Look up value in context
        std::string replacement;
        const auto* value = context.TryGetAny(placeholder_name);
        if (value) {
            replacement = AnyToString(*value);
        } else {
            // If not found, try with values map
            if (valuesIt != step.inputs.end()) {
                const std::string& valuesKey = valuesIt->second;
                const auto* valuesMap = context.TryGet<std::unordered_map<std::string, std::string>>(valuesKey);
                if (valuesMap) {
                    auto mapIt = valuesMap->find(placeholder_name);
                    if (mapIt != valuesMap->end()) {
                        replacement = mapIt->second;
                    } else {
                        throw std::runtime_error("string.format: placeholder '{" + placeholder_name + "}' not found in values map");
                    }
                } else {
                    throw std::runtime_error("string.format: placeholder '{" + placeholder_name + "}' not found");
                }
            } else {
                throw std::runtime_error("string.format: placeholder '{" + placeholder_name + "}' not found");
            }
        }

        formatted.append(replacement);
        search_start = match.suffix().first;
    }
    formatted.append(search_start, result.cend());

    // Store result in context
    context.Set(outputKey, formatted);

    if (logger_) {
        logger_->Trace("WorkflowStringFormatStep", "Execute",
                       "template_length=" + std::to_string(template_content.length()) +
                       ", result=" + formatted,
                       "String formatted successfully");
    }
}

}  // namespace sdl3cpp::services::impl
