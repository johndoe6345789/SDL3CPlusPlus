#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Orders `nodeIds` via Kahn's topological sort.
 *
 * `indexById` breaks ties among simultaneously-ready nodes by their
 * original position, so the result is deterministic. `indegree` and
 * `adjacency` are consumed (indegree is decremented in place as nodes are
 * processed).
 *
 * @throws std::runtime_error if the graph contains a cycle.
 */
std::vector<std::string> KahnTopologicalSort(
    const std::vector<std::string>& nodeIds,
    const std::unordered_map<std::string, size_t>& indexById,
    std::unordered_map<std::string, size_t> indegree,
    const std::unordered_map<std::string, std::vector<std::string>>& adjacency);

}  // namespace sdl3cpp::services::impl
