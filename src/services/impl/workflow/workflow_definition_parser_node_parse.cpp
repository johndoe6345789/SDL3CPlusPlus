#include "services/interfaces/workflow/workflow_node_parse_helpers.hpp"

namespace sdl3cpp::services::impl {

WorkflowStepDefinition ParseOneWorkflowNode(
    const WorkflowParameterReader& paramReader, const rapidjson::Value& entry,
    rapidjson::SizeType index,
    std::unordered_map<std::string, std::string>& nameToId) {
    WorkflowStepDefinition step;
    step.id     = paramReader.ReadNodeId(entry, index);
    step.plugin = paramReader.ReadNodePlugin(entry, step.id);

    // Build name->id mapping for n8n connection resolution
    if (entry.HasMember("name") && entry["name"].IsString()) {
        nameToId[entry["name"].GetString()] = step.id;
    }

    // Read inputs/outputs (top-level OR nested in parameters)
    step.inputs  = paramReader.ReadStringMap(entry, "inputs");
    step.outputs = paramReader.ReadStringMap(entry, "outputs");

    // Extract nested inputs/outputs from parameters if not at top level
    if (entry.HasMember("parameters") && entry["parameters"].IsObject()) {
        const auto& params = entry["parameters"];
        if (step.inputs.empty() && params.HasMember("inputs")) {
            step.inputs = paramReader.ReadStringMap(params, "inputs");
        }
        if (step.outputs.empty() && params.HasMember("outputs")) {
            step.outputs = paramReader.ReadStringMap(params, "outputs");
        }
    }

    step.parameters = paramReader.ReadParameterMap(entry, "parameters");
    return step;
}

}  // namespace sdl3cpp::services::impl
