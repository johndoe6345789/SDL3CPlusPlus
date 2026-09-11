#include "services/interfaces/workflow/gta5/gta5_vehicle_seat.hpp"

namespace sdl3cpp::services::impl {
namespace {

/// Sideways offset used to step out without landing inside the car.
constexpr float kExitSideways = 2.2f;
constexpr float kSeatHeight = 0.8f;

void Place(btRigidBody* player, const btVector3& where) {
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(where);
    player->setWorldTransform(transform);
    if (player->getMotionState()) {
        player->getMotionState()->setWorldTransform(transform);
    }
    player->setLinearVelocity(btVector3(0, 0, 0));
    player->setAngularVelocity(btVector3(0, 0, 0));
}

}  // namespace

int FindGta5VehicleNear(const Gta5StreamState& state, const btVector3& point,
                        float reach) {
    int best = -1;
    float nearest = reach * reach;
    for (std::size_t i = 0; i < state.vehicles.size(); ++i) {
        const btRigidBody* chassis = state.vehicles[i].chassis;
        if (!chassis) continue;
        const float distance =
            (chassis->getWorldTransform().getOrigin() - point).length2();
        if (distance < nearest) {
            nearest = distance;
            best = static_cast<int>(i);
        }
    }
    return best;
}

void RideGta5Vehicle(const Gta5Vehicle& car, btRigidBody* player) {
    if (!car.chassis || !player) return;
    // Pinned every frame rather than constrained: the player body is
    // kinematic as far as the car is concerned, and a constraint would
    // feed its mass back into the suspension. The pin is inside the
    // chassis box, though, and left colliding that overlap resolves as a
    // penetration every step and fires the car into the air -- so while
    // seated the player touches nothing.
    player->setCollisionFlags(player->getCollisionFlags() |
                              btCollisionObject::CF_NO_CONTACT_RESPONSE);
    Place(player, car.chassis->getWorldTransform().getOrigin() +
                      btVector3(0.f, kSeatHeight, 0.f));
}

void LeaveGta5Vehicle(const Gta5Vehicle& car, btRigidBody* player) {
    if (!car.chassis || !player) return;
    const btTransform& chassis = car.chassis->getWorldTransform();
    // Step out to the car's left, in its own frame, so exiting into a
    // wall is at least predictable.
    const btVector3 side = chassis.getBasis() * btVector3(kExitSideways, 0, 0);
    player->setCollisionFlags(player->getCollisionFlags() &
                              ~btCollisionObject::CF_NO_CONTACT_RESPONSE);
    Place(player, chassis.getOrigin() + side + btVector3(0.f, 1.f, 0.f));
}

}  // namespace sdl3cpp::services::impl
