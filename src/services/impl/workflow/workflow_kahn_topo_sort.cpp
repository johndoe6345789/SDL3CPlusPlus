#include "services/interfaces/workflow/workflow_kahn_topo_sort.hpp"

#include <set>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

std::vector<std::string> KahnTopologicalSort(
    const std::vector<std::string>& nodeIds,
    const std::unordered_map<std::string, size_t>& indexById,
    std::unordered_map<std::string, size_t> indegree,
    const std::unordered_map<std::string, std::vector<std::string>>&
        adjacency) {
    std::set<std::pair<size_t, std::string>> ready;
    for (const auto& nodeId : nodeIds) {
        if (indegree[nodeId] == 0u) {
            ready.emplace(indexById.at(nodeId), nodeId);
        }
    }

    std::vector<std::string> ordered;
    ordered.reserve(nodeIds.size());
    while (!ready.empty()) {
        auto it = ready.begin();
        const std::string nodeId = it->second;
        ready.erase(it);
        ordered.push_back(nodeId);

        const auto adjIt = adjacency.find(nodeId);
        if (adjIt == adjacency.end()) {
            continue;
        }
        for (const auto& next : adjIt->second) {
            auto indegreeIt = indegree.find(next);
            if (indegreeIt == indegree.end()) {
                continue;
            }
            if (--indegreeIt->second == 0u) {
                ready.emplace(indexById.at(next), next);
            }
        }
    }

    if (ordered.size() != nodeIds.size()) {
        throw std::runtime_error("Workflow connections contain a cycle");
    }

    return ordered;
}

}  // namespace sdl3cpp::services::impl
