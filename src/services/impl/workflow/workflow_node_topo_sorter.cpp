#include "services/interfaces/workflow/workflow_node_topo_sorter.hpp"
#include "services/interfaces/workflow/workflow_connection_graph_builder.hpp"
#include "services/interfaces/workflow/workflow_kahn_topo_sort.hpp"

namespace sdl3cpp::services::impl {

WorkflowNodeTopoSorter::WorkflowNodeTopoSorter(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowNodeTopoSorter", "Constructor", "Entry");
    }
}

std::vector<std::string> WorkflowNodeTopoSorter::SortNodesByConnections(
    const std::vector<std::string>& nodeIds,
    const std::unordered_map<std::string, std::string>& nameToId,
    const std::vector<std::pair<std::string, std::string>>& edges) const {
    if (logger_) {
        logger_->Trace("WorkflowNodeTopoSorter", "SortNodesByConnections",
                       "Entry");
    }

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

    AddEdgesToGraph(edges, nameToId, indexById, adjacency, indegree);

    return KahnTopologicalSort(nodeIds, indexById, std::move(indegree),
                               adjacency);
}

}  // namespace sdl3cpp::services::impl
