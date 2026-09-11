#include "services/interfaces/workflow/gta5/gta5_ground_probe.hpp"

#include <algorithm>
#include <cstdio>

namespace sdl3cpp::services::impl {

std::string DescribeGta5Column(btDiscreteDynamicsWorld* world, float x,
                               float z) {
    if (!world) return "no physics world";
    int covering = 0;
    int scaled = 0;
    float low = 1e9f;
    float high = -1e9f;
    const btCollisionObjectArray& objects = world->getCollisionObjectArray();
    for (int i = 0; i < objects.size(); ++i) {
        btVector3 lo, hi;
        objects[i]->getCollisionShape()->getAabb(
            objects[i]->getWorldTransform(), lo, hi);
        if (x < lo.x() || x > hi.x() || z < lo.z() || z > hi.z()) continue;
        ++covering;
        if (objects[i]->getCollisionShape()->getShapeType() ==
            SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE) {
            ++scaled;
        }
        low = std::min(low, lo.y());
        high = std::max(high, hi.y());
    }
    char text[160];
    std::snprintf(text, sizeof(text),
                  "%d objects in the world, %d cover the column (%d scaled),"
                  " spanning y %.1f..%.1f",
                  objects.size(), covering, scaled, low, high);
    return text;
}

}  // namespace sdl3cpp::services::impl
