#pragma once

#include "services/interfaces/i_logger.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

/// Small focused class: orders workflow nodes by their connection edges via
/// a topological sort (Kahn's algorithm). Single job: given the node IDs, a
/// name→id lookup (n8n's connections reference nodes by name, not id), and
/// the (from, to) edges, produce a dependency-respecting node order.
class WorkflowNodeTopoSorter {
public:
    explicit WorkflowNodeTopoSorter(
        std::shared_ptr<ILogger> logger = nullptr);

    /// Sort nodes by connections using topological sort.
    /// Uses name→id mapping to resolve n8n format (connections use names,
    /// nodes use IDs). Returns ordered list of node IDs.
    std::vector<std::string> SortNodesByConnections(
        const std::vector<std::string>& nodeIds,
        const std::unordered_map<std::string, std::string>& nameToId,
        const std::vector<std::pair<std::string, std::string>>& edges) const;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
