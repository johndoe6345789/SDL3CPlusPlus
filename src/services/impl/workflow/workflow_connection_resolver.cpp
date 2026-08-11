#include "services/interfaces/workflow/workflow_connection_resolver.hpp"

#include <set>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowConnectionResolver::WorkflowConnectionResolver(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowConnectionResolver", "Constructor", "Entry");
    }
}

std::vector<std::pair<std::string, std::string>> WorkflowConnectionResolver::ReadConnections(
    const rapidjson::Value& document) const {
    if (logger_) {
        logger_->Trace("WorkflowConnectionResolver", "ReadConnections", "Entry");
    }

    if (!document.HasMember("connections")) {
        return {};
    }
    const auto& connectionsValue = document["connections"];
    if (!connectionsValue.IsObject()) {
        throw std::runtime_error("Workflow 'connections' must be an object");
    }

    std::vector<std::pair<std::string, std::string>> edges;
    for (auto it = connectionsValue.MemberBegin(); it != connectionsValue.MemberEnd(); ++it) {
        const std::string fromNode = it->name.GetString();
        if (!it->value.IsObject()) {
            throw std::runtime_error("Workflow connections for '" + fromNode + "' must be an object");
        }
        if (!it->value.HasMember("main")) {
            continue;
        }
        const auto& mainValue = it->value["main"];

        // Support both n8n format (object with numeric keys) and simple array format
        if (mainValue.IsObject()) {
            // n8n format: "main": { "0": [...], "1": [...] }
            for (auto branchIt = mainValue.MemberBegin(); branchIt != mainValue.MemberEnd(); ++branchIt) {
                if (!branchIt->value.IsArray()) {
                    throw std::runtime_error("Workflow connections.main[" + std::string(branchIt->name.GetString()) +
                                           "] for '" + fromNode + "' must be an array");
                }
                for (const auto& connection : branchIt->value.GetArray()) {
                    if (!connection.IsObject() || !connection.HasMember("node") || !connection["node"].IsString()) {
                        throw std::runtime_error("Workflow connection entries for '" + fromNode + "' require a node string");
                    }
                    edges.emplace_back(fromNode, connection["node"].GetString());
                }
            }
        } else if (mainValue.IsArray()) {
            // Simple array format: "main": [[...]]
            for (const auto& branch : mainValue.GetArray()) {
                if (!branch.IsArray()) {
                    throw std::runtime_error("Workflow connections.main entries for '" + fromNode + "' must be arrays");
                }
                for (const auto& connection : branch.GetArray()) {
                    if (!connection.IsObject() || !connection.HasMember("node") || !connection["node"].IsString()) {
                        throw std::runtime_error("Workflow connection entries for '" + fromNode + "' require a node string");
                    }
                    edges.emplace_back(fromNode, connection["node"].GetString());
                }
            }
        } else {
            throw std::runtime_error("Workflow connections.main for '" + fromNode + "' must be an object or array");
        }
    }
    return edges;
}

std::vector<std::string> WorkflowConnectionResolver::SortNodesByConnections(
    const std::vector<std::string>& nodeIds,
    const std::unordered_map<std::string, std::string>& nameToId,
    const std::vector<std::pair<std::string, std::string>>& edges) const {
    if (logger_) {
        logger_->Trace("WorkflowConnectionResolver", "SortNodesByConnections", "Entry");
    }

    // Build graph structures
    std::unordered_map<std::string, size_t> indexById;
    std::unordered_map<std::string, size_t> indegree;
    std::unordered_map<std::string, std::vector<std::string>> adjacency;
    indexById.reserve(nodeIds.size());
    indegree.reserve(nodeIds.size());
    adjacency.reserve(nodeIds.size());

    for (size_t i = 0; i < nodeIds.size(); ++i) {
        indexById.emplace(nodeIds[i], i);
        indegree.emplace(nodeIds[i], 0);
        adjacency.emplace(nodeIds[i], std::vector<std::string>{});
    }

    // Add edges (resolve names to IDs)
    for (const auto& edge : edges) {
        std::string fromId = edge.first;
        std::string toId = edge.second;

        // Resolve node names to IDs (n8n uses names in connections)
        auto fromNameIt = nameToId.find(edge.first);
        if (fromNameIt != nameToId.end()) {
            fromId = fromNameIt->second;
        }
        auto toNameIt = nameToId.find(edge.second);
        if (toNameIt != nameToId.end()) {
            toId = toNameIt->second;
        }

        // Validate nodes exist
        const auto fromIt = indexById.find(fromId);
        if (fromIt == indexById.end()) {
            throw std::runtime_error("Workflow connection references unknown node '" + edge.first + "' (id: " + fromId + ")");
        }
        const auto toIt = indexById.find(toId);
        if (toIt == indexById.end()) {
            throw std::runtime_error("Workflow connection references unknown node '" + edge.second + "' (id: " + toId + ")");
        }

        adjacency[fromId].push_back(toId);
        ++indegree[toId];
    }

    // Topological sort (Kahn's algorithm)
    std::set<std::pair<size_t, std::string>> ready;
    for (const auto& nodeId : nodeIds) {
        if (indegree[nodeId] == 0u) {
            ready.emplace(indexById[nodeId], nodeId);
        }
    }

    std::vector<std::string> ordered;
    ordered.reserve(nodeIds.size());
    while (!ready.empty()) {
        auto it = ready.begin();
        const std::string nodeId = it->second;
        ready.erase(it);
        ordered.push_back(nodeId);

        for (const auto& next : adjacency[nodeId]) {
            auto indegreeIt = indegree.find(next);
            if (indegreeIt == indegree.end()) {
                continue;
            }
            if (--indegreeIt->second == 0u) {
                ready.emplace(indexById[next], next);
            }
        }
    }

    if (ordered.size() != nodeIds.size()) {
        throw std::runtime_error("Workflow connections contain a cycle");
    }

    return ordered;
}

}  // namespace sdl3cpp::services::impl
