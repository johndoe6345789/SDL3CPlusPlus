#pragma once

#include "services/interfaces/i_logger.hpp"

#include <rapidjson/document.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

/// Small focused class: Resolve workflow connections and perform topological sort
/// Single job: Parse n8n connections format and order nodes by dependencies
/// Keeps connection resolution logic under 100 LOC
class WorkflowConnectionResolver {
public:
    explicit WorkflowConnectionResolver(std::shared_ptr<ILogger> logger = nullptr);
    /// Read connections from workflow JSON (n8n format)
    /// Returns edges as (from, to) pairs using node names
    std::vector<std::pair<std::string, std::string>> ReadConnections(
        const rapidjson::Value& document) const;

    /// Sort nodes by connections using topological sort
    /// Uses name→id mapping to resolve n8n format (connections use names, nodes use IDs)
    /// Returns ordered list of node IDs
    std::vector<std::string> SortNodesByConnections(
        const std::vector<std::string>& nodeIds,
        const std::unordered_map<std::string, std::string>& nameToId,
        const std::vector<std::pair<std::string, std::string>>& edges) const;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
