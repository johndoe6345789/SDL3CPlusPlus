#include "services/interfaces/workflow/gta5/gta5_vehicle.hpp"

#include "services/interfaces/workflow/gta5/gta5_vehicle_body.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_wheels.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_load.hpp"

namespace sdl3cpp::services::impl {
bool SpawnGta5Vehicle(Gta5StreamState& state, const Gta5VehicleSpec& spec,
                      const glm::vec3& position, float mass,
                      SDL_GPUDevice* device, btDiscreteDynamicsWorld* world,
                      const std::shared_ptr<ILogger>& logger) {
    if (!device || !world) return false;

    Gta5Vehicle car;
    Gta5WheelSetup setup;
    if (!LoadGta5VehicleChassis(state, spec, device, car, setup, logger)) {
        return false;
    }
    Gta5Geometry& geometry = *car.instance.geometry;
    car.chassis = MakeGta5VehicleBody(geometry, position, mass,
                                      car.chassisShape);
    world->addRigidBody(car.chassis);
    car.instance.body = car.chassis;

    LoadGta5VehicleWheelMeshes(state, spec, device, car, setup, logger);

    // The chassis shape is a compound wrapping the box, so reach
    // through it for the extents the fallback wheel layout needs.
    const auto* compound = static_cast<btCompoundShape*>(car.chassisShape);
    const btVector3 half =
        static_cast<const btBoxShape*>(compound->getChildShape(0))
            ->getHalfExtentsWithMargin();
    AttachGta5Wheels(car, world, half, setup);

    // Held by the vehicle list, so the geometry sweep cannot free the
    // mesh from under a car still in the world.
    ++geometry.references;
    state.vehicles.push_back(car);

    if (logger) {
        logger->Info("gta5.vehicle.spawn: " + spec.model + " with " +
                     std::to_string(car.vehicle->getNumWheels()) +
                     " wheels, " + (car.hasWheels ? spec.wheel : "none") +
                     " drawn");
    }
    return true;
}

void UpdateGta5Vehicles(Gta5StreamState& state) {
    for (Gta5Vehicle& car : state.vehicles) {
        if (!car.vehicle) continue;
        car.vehicle->getChassisWorldTransform().getOpenGLMatrix(
            car.instance.modelMatrix.data());
        // Each wheel transform carries the steer angle and the rolling
        // rotation Bullet integrated, so placing the mesh from it is all
        // the turning and steering there is to do.
        for (int i = 0; i < car.vehicle->getNumWheels(); ++i) {
            car.vehicle->updateWheelTransform(i, true);
            if (car.hasWheels && i < 4) {
                car.vehicle->getWheelTransformWS(i).getOpenGLMatrix(
                    car.wheels[i].modelMatrix.data());
            }
        }
    }
}

}  // namespace sdl3cpp::services::impl
