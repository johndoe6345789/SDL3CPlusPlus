#include "services/interfaces/workflow/quake3/q3_nav_sample.hpp"

namespace sdl3cpp::services::impl {

sdl3cpp::q3::NavGraphPtr SampleNavGraph(btDiscreteDynamicsWorld* world,
                                        const NavBuildAabb& aabb) {
    constexpr float kStep        = 2.0f;  // grid spacing in XZ
    constexpr float kRayUp       = 50.f;  // start ray this far above point
    constexpr float kRayDown     = 50.f;  // ray extends this far below it
    constexpr float kMinNormalY  = 0.7f;  // walkable surface normal min
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

}  // namespace sdl3cpp::services::impl
