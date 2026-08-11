#include "services/interfaces/workflow/workflow_definition_parser.hpp"
#include "services/interfaces/workflow/workflow_parameter_reader.hpp"
#include "services/interfaces/workflow/workflow_connection_resolver.hpp"

#include <rapidjson/document.h>

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services::impl {

std::vector<WorkflowStepDefinition> WorkflowDefinitionParser::ParseNodes(
    const rapidjson::Document& document) const {
    if (logger_) {
        logger_->Trace("WorkflowDefinitionParser", "ParseNodes", "Entry");
    }

    if (!document["nodes"].IsArray()) {
        throw std::runtime_error("Workflow must contain a 'nodes' array");
    }

    WorkflowParameterReader paramReader;
    std::vector<WorkflowStepDefinition> nodes;
    std::vector<std::string> nodeOrder;
    std::unordered_map<std::string, std::string> nameToId;  // n8n uses names in connections

    // Parse all nodes
    for (rapidjson::SizeType i = 0; i < document["nodes"].Size(); ++i) {
        const auto& entry = document["nodes"][i];
        if (!entry.IsObject()) {
            throw std::runtime_error("Workflow nodes must be objects");
        }

        WorkflowStepDefinition step;
        step.id = paramReader.ReadNodeId(entry, i);
        step.plugin = paramReader.ReadNodePlugin(entry, step.id);

        // Build name->id mapping for n8n connection resolution
        if (entry.HasMember("name") && entry["name"].IsString()) {
            nameToId[entry["name"].GetString()] = step.id;
        }

        // Read inputs/outputs (top-level OR nested in parameters)
        step.inputs = paramReader.ReadStringMap(entry, "inputs");
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
        nodes.push_back(step);
        nodeOrder.push_back(step.id);
    }

    // Resolve connections and sort nodes
    WorkflowConnectionResolver connResolver;
    const auto edges = connResolver.ReadConnections(document);

    std::vector<std::string> orderedIds = edges.empty()
        ? nodeOrder
        : connResolver.SortNodesByConnections(nodeOrder, nameToId, edges);

    // Build final workflow with sorted nodes
    std::unordered_map<std::string, WorkflowStepDefinition> nodeMap;
    nodeMap.reserve(nodes.size());
    for (const auto& node : nodes) {
        nodeMap.emplace(node.id, node);
    }

    std::vector<WorkflowStepDefinition> sortedSteps;
    sortedSteps.reserve(nodes.size());
    for (const auto& nodeId : orderedIds) {
        auto it = nodeMap.find(nodeId);
        if (it == nodeMap.end()) {
            throw std::runtime_error("Workflow nodes missing entry for '" + nodeId + "'");
        }
        sortedSteps.push_back(it->second);
    }

    return sortedSteps;
}

}  // namespace sdl3cpp::services::impl
