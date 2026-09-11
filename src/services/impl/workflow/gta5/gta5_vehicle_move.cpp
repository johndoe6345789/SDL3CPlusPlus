#include "services/interfaces/workflow/gta5/gta5_vehicle_seat.hpp"

#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>

#include <cmath>

namespace sdl3cpp::services::impl {

void MoveGta5Vehicle(Gta5Vehicle& car, const glm::vec3& at) {
    if (!car.chassis) return;
    // Its heading kept, levelled: however it lay, it lands on its wheels.
    const btVector3 ahead =
        car.chassis->getWorldTransform().getBasis() * btVector3(0, 0, 1);
    const float yaw = std::atan2(ahead.x(), ahead.z());
    const btTransform to(btQuaternion(btVector3(0.f, 1.f, 0.f), yaw),
                         btVector3(at.x, at.y, at.z));
    car.chassis->setWorldTransform(to);
    if (car.chassis->getMotionState()) {
        car.chassis->getMotionState()->setWorldTransform(to);
    }
    car.chassis->setLinearVelocity(btVector3(0.f, 0.f, 0.f));
    car.chassis->setAngularVelocity(btVector3(0.f, 0.f, 0.f));
    car.chassis->activate(true);
    car.heldAt = to;  // held or not, it is here now
    if (car.vehicle) car.vehicle->resetSuspension();
}

}  // namespace sdl3cpp::services::impl
