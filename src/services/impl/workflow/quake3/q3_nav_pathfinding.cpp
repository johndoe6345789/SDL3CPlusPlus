#include "services/interfaces/workflow/quake3/q3_nav_pathfinding.hpp"

#include <algorithm>
#include <cmath>
#include <queue>
#include <utility>

namespace sdl3cpp::q3 {
namespace {

float Distance(const glm::vec3& a, const glm::vec3& b) {
    const glm::vec3 d = a - b;
    return std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
}

}  // namespace

std::vector<int> FindNavPath(const NavGraph& graph, int startNode,
                             int goalNode) {
    const int nodeCount = static_cast<int>(graph.nodes.size());
    if (startNode < 0 || goalNode < 0 || startNode >= nodeCount ||
        goalNode >= nodeCount) {
        return {};
    }
    if (startNode == goalNode) {
        return {startNode};
    }

    std::vector<float> gCost(nodeCount, 1e9f);
    std::vector<int> parent(nodeCount, -1);

    // Min-heap of (fCost, nodeIndex).
    using Entry = std::pair<float, int>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;

    gCost[startNode] = 0.0f;
    open.push({Distance(graph.nodes[startNode].pos, graph.nodes[goalNode].pos),
               startNode});

    while (!open.empty()) {
        const auto [f, current] = open.top();
        open.pop();
        if (current == goalNode) {
            break;
        }

        for (int neighbor : graph.nodes[current].neighbors) {
            const float edgeCost =
                Distance(graph.nodes[neighbor].pos, graph.nodes[current].pos);
            const float newG = gCost[current] + edgeCost;
            if (newG < gCost[neighbor]) {
                gCost[neighbor]  = newG;
                parent[neighbor] = current;
                open.push({newG + Distance(graph.nodes[neighbor].pos,
                                           graph.nodes[goalNode].pos),
                           neighbor});
            }
        }
    }

    if (parent[goalNode] == -1 && goalNode != startNode) {
        return {};
    }

    std::vector<int> path;
    for (int current = goalNode; current != -1; current = parent[current]) {
        path.push_back(current);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

int NearestNavNode(const NavGraph& graph, const glm::vec3& pos) {
    int best         = -1;
    float bestDistSq = 1e9f;
    for (int i = 0; i < static_cast<int>(graph.nodes.size()); ++i) {
        const glm::vec3 d  = graph.nodes[i].pos - pos;
        const float distSq = d.x * d.x + d.y * d.y + d.z * d.z;
        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            best       = i;
        }
    }
    return best;
}

}  // namespace sdl3cpp::q3
