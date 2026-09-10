#include "services/interfaces/workflow/quake3/q3_nav_build.hpp"

namespace sdl3cpp::services::impl {

NavBuildAabb ComputeNavBuildAabb(const nlohmann::json* spawnPts) {
    NavBuildAabb aabb{glm::vec3(-50.f, -10.f, -50.f),
                      glm::vec3(50.f, 50.f, 50.f)};
    if (!spawnPts || !spawnPts->is_array() || spawnPts->empty()) return aabb;

    glm::vec3 mn(1e9f), mx(-1e9f);
    bool any = false;
    for (const auto& sp : *spawnPts) {
        // Accept either {pos:[x,y,z]} or direct [x,y,z] arrays
        const nlohmann::json* posJ = nullptr;
        if (sp.is_array() && sp.size() >= 3) posJ = &sp;
        else if (sp.contains("position") && sp["position"].is_array())
            posJ = &sp["position"];
        else if (sp.contains("pos") && sp["pos"].is_array())
            posJ = &sp["pos"];
        if (!posJ) continue;

        glm::vec3 p((*posJ)[0].get<float>(), (*posJ)[1].get<float>(),
                    (*posJ)[2].get<float>());
        mn = glm::min(mn, p);
        mx = glm::max(mx, p);
        any = true;
    }
    if (any) {
        // Expand by 20 units to cover the full playable area around spawns
        constexpr float kPad = 20.f;
        aabb.min = mn - glm::vec3(kPad, 5.f, kPad);
        aabb.max = mx + glm::vec3(kPad, 5.f, kPad);
    }
    return aabb;
}

sdl3cpp::q3::NavGraphPtr SampleNavGraph(
    btDiscreteDynamicsWorld* world, const NavBuildAabb& aabb) {
    constexpr float kStep = 2.0f;         // grid spacing in XZ
    constexpr float kRayUp = 50.f;        // start ray this far above point
    constexpr float kRayDown = 50.f;      // ray extends this far below it
    constexpr float kMinNormalY = 0.7f;   // walkable surface normal min
    constexpr float kAgentHeight = 0.5f;  // lift node above hit point

    auto graph = std::make_shared<sdl3cpp::q3::NavGraph>();

    for (float x = aabb.min.x; x <= aabb.max.x; x += kStep) {
        for (float z = aabb.min.z; z <= aabb.max.z; z += kStep) {
            const float midY = (aabb.min.y + aabb.max.y) * 0.5f;
            const btVector3 from(x, midY + kRayUp, z);
            const btVector3 to(x, midY - kRayDown, z);

            btCollisionWorld::ClosestRayResultCallback cb(from, to);
            world->rayTest(from, to, cb);

            if (!cb.hasHit()) continue;
            if (cb.m_hitNormalWorld.y() < kMinNormalY) continue;

            sdl3cpp::q3::NavNode node;
            node.pos = glm::vec3(cb.m_hitPointWorld.x(),
                cb.m_hitPointWorld.y() + kAgentHeight,
                cb.m_hitPointWorld.z());
            graph->nodes.push_back(node);
        }
    }
    return graph;
}

void ConnectNavNeighbors(
    btDiscreteDynamicsWorld* world, sdl3cpp::q3::NavGraph& graph) {
    constexpr float kNeighborDist = 3.0f;
    constexpr float kNeighborDist2 = kNeighborDist * kNeighborDist;

    const int n = static_cast<int>(graph.nodes.size());
    for (int i = 0; i < n; ++i) {
        const glm::vec3& pi = graph.nodes[i].pos;
        for (int j = i + 1; j < n; ++j) {
            const glm::vec3& pj = graph.nodes[j].pos;
            const glm::vec3 d = pj - pi;
            const float dist2 = d.x * d.x + d.y * d.y + d.z * d.z;
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
