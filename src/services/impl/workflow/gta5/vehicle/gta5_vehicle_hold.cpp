#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_hold.hpp"


namespace sdl3cpp::services::impl {
namespace {

/// Nearest hit, skipping one object and anything without contact response.
struct IgnoringRay : btCollisionWorld::ClosestRayResultCallback {
    IgnoringRay(const btVector3& from, const btVector3& to,
                const btCollisionObject* skip)
        : ClosestRayResultCallback(from, to), skip_(skip) {}
    bool needsCollision(btBroadphaseProxy* proxy) const override {
        const auto* object =
            static_cast<const btCollisionObject*>(proxy->m_clientObject);
        return object != skip_ && object->hasContactResponse() &&
               ClosestRayResultCallback::needsCollision(proxy);
    }
    const btCollisionObject* skip_;
};

}  // namespace

void HoldGta5VehiclesOverMissingGround(Gta5StreamState& state,
                                       btDiscreteDynamicsWorld* world) {
    for (Gta5Vehicle& car : state.vehicles) {
        if (!car.chassis) continue;
        const btVector3 at = car.chassis->getWorldTransform().getOrigin();
        // From the chassis itself: started a metre under it, the ray began
        // below the road once the car settled on its wheels, found nothing,
        // and froze the car where it stood.
        float ground = 0.f;
        if (Gta5GroundUnder(world, car.chassis, ground)) {
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

bool Gta5GroundUnder(btDiscreteDynamicsWorld* world,
                     const btCollisionObject* self, float& groundY) {
    if (!world || !self) return false;
    const btVector3 from = self->getWorldTransform().getOrigin();
    const btVector3 to = from - btVector3(0.f, 500.f, 0.f);
    IgnoringRay hit(from, to, self);
    world->rayTest(from, to, hit);
    if (!hit.hasHit()) return false;
    groundY = hit.m_hitPointWorld.y();
    return true;
}

}  // namespace sdl3cpp::services::impl
