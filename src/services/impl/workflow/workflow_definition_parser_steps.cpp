#include "services/interfaces/workflow/workflow_definition_parser.hpp"

#include <rapidjson/document.h>

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

std::vector<WorkflowStepDefinition> WorkflowDefinitionParser::ParseStepsFormat(
    const rapidjson::Document& document,
    const WorkflowParameterReader& paramReader) const {
    if (!document["steps"].IsArray()) {
        throw std::runtime_error("Workflow must contain a 'steps' array");
    }

    std::vector<WorkflowStepDefinition> steps;
    for (const auto& entry : document["steps"].GetArray()) {
        if (!entry.IsObject()) {
            throw std::runtime_error("Workflow steps must be objects");
        }
        WorkflowStepDefinition step;
        step.id = paramReader.ReadRequiredString(entry, "id");
        step.plugin = paramReader.ReadRequiredString(entry, "plugin");
        step.inputs = paramReader.ReadStringMap(entry, "inputs");
        step.outputs = paramReader.ReadStringMap(entry, "outputs");
        step.parameters = paramReader.ReadParameterMap(entry, "parameters");
        steps.push_back(std::move(step));
    }
    return steps;
}

}  // namespace sdl3cpp::services::impl
