#include "services/interfaces/workflow/gta5/gta5_vehicle_hold.hpp"


namespace sdl3cpp::services::impl {

void HoldGta5VehiclesOverMissingGround(Gta5StreamState& state,
                                       btDiscreteDynamicsWorld* world) {
    for (Gta5Vehicle& car : state.vehicles) {
        if (!car.chassis) continue;
        const btVector3 at = car.chassis->getWorldTransform().getOrigin();
        // Asked from 6 m down, the search starts a metre under the car,
        // below the chassis box, whose floor is at axle height.
        float ground = 0.f;
        if (Gta5GroundBelow(world, glm::vec3(at.x(), at.y() - 6.f, at.z()),
                            ground)) {
            car.held = false;
            continue;
        }
        if (!car.held) {
            car.heldAt = car.chassis->getWorldTransform();
            car.held = true;
        }
        car.chassis->setWorldTransform(car.heldAt);
        if (car.chassis->getMotionState()) {
            car.chassis->getMotionState()->setWorldTransform(car.heldAt);
        }
        car.chassis->setLinearVelocity(btVector3(0.f, 0.f, 0.f));
        car.chassis->setAngularVelocity(btVector3(0.f, 0.f, 0.f));
    }
}

bool Gta5GroundBelow(btDiscreteDynamicsWorld* world, const glm::vec3& from,
                     float& groundY) {
    if (!world) return false;
    // Just above the given height, not high above it: a spawn on a lane
    // under an overpass met the overpass first and landed on top of it,
    // 17 m above the player.
    const btVector3 top(from.x, from.y + 5.f, from.z);
    const btVector3 bottom(from.x, from.y - 500.f, from.z);
    btCollisionWorld::ClosestRayResultCallback hit(top, bottom);
    world->rayTest(top, bottom, hit);
    if (!hit.hasHit()) return false;
    groundY = hit.m_hitPointWorld.y();
    return true;
}

}  // namespace sdl3cpp::services::impl
