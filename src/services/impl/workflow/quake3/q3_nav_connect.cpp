#include "services/interfaces/workflow/quake3/q3_nav_connect.hpp"

namespace sdl3cpp::services::impl {

void ConnectNavNeighbors(btDiscreteDynamicsWorld* world,
                         sdl3cpp::q3::NavGraph& graph) {
    constexpr float kNeighborDist  = 3.0f;
    constexpr float kNeighborDist2 = kNeighborDist * kNeighborDist;

    const int n = static_cast<int>(graph.nodes.size());
    for (int i = 0; i < n; ++i) {
        const glm::vec3& pi = graph.nodes[i].pos;
        for (int j = i + 1; j < n; ++j) {
            const glm::vec3& pj = graph.nodes[j].pos;
            const glm::vec3 d   = pj - pi;
            const float dist2   = d.x * d.x + d.y * d.y + d.z * d.z;
            if (dist2 > kNeighborDist2) continue;

            // LOS check
            btVector3 from(pi.x, pi.y, pi.z);
            btVector3 to(pj.x, pj.y, pj.z);
            btCollisionWorld::ClosestRayResultCallback cb(from, to);
            world->rayTest(from, to, cb);

            if (!cb.hasHit()) {
                graph.nodes[i].neighbors.push_back(j);
                graph.nodes[j].neighbors.push_back(i);
            }
        }
    }
}

}  // namespace sdl3cpp::services::impl
