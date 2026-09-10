#include "services/interfaces/workflow/workflow_node_parse_helpers.hpp"
#include "services/interfaces/workflow/workflow_connection_resolver.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

std::vector<WorkflowStepDefinition> SortWorkflowNodes(
    const rapidjson::Document& document,
    const std::vector<WorkflowStepDefinition>& nodes,
    const std::vector<std::string>& nodeOrder,
    const std::unordered_map<std::string, std::string>& nameToId) {
    WorkflowConnectionResolver connResolver;
    const auto edges = connResolver.ReadConnections(document);

    std::vector<std::string> orderedIds = edges.empty()
        ? nodeOrder
        : connResolver.SortNodesByConnections(nodeOrder, nameToId, edges);

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
            throw std::runtime_error("Workflow nodes missing entry for '" +
                                     nodeId + "'");
        }
        sortedSteps.push_back(it->second);
    }

    return sortedSteps;
}

}  // namespace sdl3cpp::services::impl
