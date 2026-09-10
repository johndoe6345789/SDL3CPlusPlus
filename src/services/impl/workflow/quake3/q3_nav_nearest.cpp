#include "services/interfaces/workflow/quake3/q3_nav_nearest.hpp"

namespace sdl3cpp::q3 {

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
