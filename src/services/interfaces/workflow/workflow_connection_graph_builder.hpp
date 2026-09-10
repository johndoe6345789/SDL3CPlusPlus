#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Adds each edge to `adjacency`/`indegree`, resolving n8n's
 *        name-based endpoints to node IDs via `nameToId`.
 * @throws std::runtime_error if either endpoint does not resolve to a
 *         known node ID (a key in `indexById`).
 */
void AddEdgesToGraph(
    const std::vector<std::pair<std::string, std::string>>& edges,
    const std::unordered_map<std::string, std::string>& nameToId,
    const std::unordered_map<std::string, size_t>& indexById,
    std::unordered_map<std::string, std::vector<std::string>>& adjacency,
    std::unordered_map<std::string, size_t>& indegree);

}  // namespace sdl3cpp::services::impl
