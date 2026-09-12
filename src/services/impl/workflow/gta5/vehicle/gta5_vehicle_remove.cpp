#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_seat.hpp"

#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>

#include <cstddef>

namespace sdl3cpp::services::impl {

void DestroyGta5Vehicle(Gta5Vehicle& car,
                        btDiscreteDynamicsWorld* world) {
    if (world && car.vehicle) world->removeAction(car.vehicle);
    if (world && car.chassis) world->removeRigidBody(car.chassis);
    delete car.vehicle;
    delete car.raycaster;
    if (car.chassis) delete car.chassis->getMotionState();
    delete car.chassis;
    // A compound does not own its children: the box inside goes too.
    if (car.chassisShape &&
        car.chassisShape->getShapeType() == COMPOUND_SHAPE_PROXYTYPE) {
        auto* compound = static_cast<btCompoundShape*>(car.chassisShape);
        for (int i = compound->getNumChildShapes() - 1; i >= 0; --i) {
            delete compound->getChildShape(i);
        }
    }
    delete car.chassisShape;
    car.vehicle = nullptr;
    car.raycaster = nullptr;
    car.chassis = nullptr;
    car.chassisShape = nullptr;
    // Its meshes stay cached for the next such car; only the counts drop.
    if (car.instance.geometry) --car.instance.geometry->references;
    for (Gta5Instance& wheel : car.wheels) {
        if (car.hasWheels && wheel.geometry) --wheel.geometry->references;
    }
    car.instance.geometry = nullptr;
    car.hasWheels = false;
}

void RemoveGta5Vehicle(Gta5StreamState& state, std::size_t index,
                       btDiscreteDynamicsWorld* world) {
    if (index >= state.vehicles.size()) return;
    DestroyGta5Vehicle(state.vehicles[index], world);
    if (state.seated == static_cast<int>(index)) state.seated = -1;
    state.vehicles.erase(state.vehicles.begin() +
                         static_cast<std::ptrdiff_t>(index));
}

void RepaintGta5Vehicle(Gta5Vehicle& car, const glm::vec3& paint) {
    if (!car.instance.geometry) return;
    for (Gta5SubMesh& sub : car.instance.geometry->subMeshes) {
        if (!sub.paint) continue;
        sub.surface[0] = paint.r;
        sub.surface[1] = paint.g;
        sub.surface[2] = paint.b;
    }
}

}  // namespace sdl3cpp::services::impl
