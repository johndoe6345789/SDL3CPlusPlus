#include "services/interfaces/workflow/workflow_connection_graph_builder.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {
namespace {

/// Resolves an edge's endpoint name to a node ID via `nameToId` (n8n
/// connections reference nodes by name), falling back to the name itself
/// when it is already an ID.
std::string ResolveNodeId(
    const std::string& nameOrId,
    const std::unordered_map<std::string, std::string>& nameToId) {
    const auto it = nameToId.find(nameOrId);
    return it != nameToId.end() ? it->second : nameOrId;
}

}  // namespace

void AddEdgesToGraph(
    const std::vector<std::pair<std::string, std::string>>& edges,
    const std::unordered_map<std::string, std::string>& nameToId,
    const std::unordered_map<std::string, size_t>& indexById,
    std::unordered_map<std::string, std::vector<std::string>>& adjacency,
    std::unordered_map<std::string, size_t>& indegree) {
    for (const auto& edge : edges) {
        const std::string fromId = ResolveNodeId(edge.first, nameToId);
        const std::string toId   = ResolveNodeId(edge.second, nameToId);

        if (indexById.find(fromId) == indexById.end()) {
            throw std::runtime_error(
                "Workflow connection references unknown node '" +
                edge.first + "' (id: " + fromId + ")");
        }
        if (indexById.find(toId) == indexById.end()) {
            throw std::runtime_error(
                "Workflow connection references unknown node '" +
                edge.second + "' (id: " + toId + ")");
        }

        adjacency[fromId].push_back(toId);
        ++indegree[toId];
    }
}

}  // namespace sdl3cpp::services::impl
