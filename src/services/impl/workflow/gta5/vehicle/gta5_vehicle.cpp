#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle.hpp"

#include "services/interfaces/workflow/gta5/player/gta5_shown_transform.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_body.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_wheels.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_load.hpp"

namespace sdl3cpp::services::impl {
bool MakeGta5Vehicle(Gta5StreamState& state, const Gta5VehicleSpec& spec,
                     const glm::vec3& position, float mass,
                     SDL_GPUDevice* device, btDiscreteDynamicsWorld* world,
                     Gta5Vehicle& car,
                     const std::shared_ptr<ILogger>& logger) {
    if (!device || !world) return false;

    Gta5WheelSetup setup;
    if (!LoadGta5VehicleChassis(state, spec, device, car, setup, logger)) {
        return false;
    }
    Gta5Geometry& geometry = *car.instance.geometry;
    // The box floor starts 10 cm above the axles. Sized from the whole
    // mesh it reached down to the brake discs, 8 cm above where the wheels
    // rest, and beached the car on the first kerb, front wheels in the air.
    car.chassis = MakeGta5VehicleBody(
        geometry, position, mass, car.chassisShape,
        setup.hasAxles ? setup.axles[0].y() + 0.1f : -1.0e9f);
    btTransform facing = car.chassis->getWorldTransform();
    facing.setRotation(
        btQuaternion(btVector3(0.f, 1.f, 0.f), glm::radians(spec.heading)));
    car.chassis->setWorldTransform(facing);
    car.chassis->getMotionState()->setWorldTransform(facing);
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

    if (logger) {
        logger->Info("gta5.vehicle.spawn: " + spec.model + " with " +
                     std::to_string(car.vehicle->getNumWheels()) +
                     " wheels, " + (car.hasWheels ? spec.wheel : "none") +
                     " drawn");
    }
    return true;
}

bool SpawnGta5Vehicle(Gta5StreamState& state, const Gta5VehicleSpec& spec,
                      const glm::vec3& position, float mass,
                      SDL_GPUDevice* device, btDiscreteDynamicsWorld* world,
                      const std::shared_ptr<ILogger>& logger) {
    Gta5Vehicle car;
    if (!MakeGta5Vehicle(state, spec, position, mass, device, world, car,
                         logger)) {
        return false;
    }
    state.vehicles.push_back(car);
    return true;
}

void UpdateGta5Vehicle(Gta5Vehicle& car) {
    if (!car.vehicle) return;
    // Drawn interpolated between physics steps; the wheels, which
    // Bullet places from the last step, move by the same offset.
    const btTransform shown = Gta5ShownTransform(car.chassis);
    shown.getOpenGLMatrix(car.instance.modelMatrix.data());
    const btTransform offset =
        shown * car.chassis->getWorldTransform().inverse();
    // Each wheel transform carries the steer angle and the rolling
    // rotation Bullet integrated: all the turning there is to do.
    for (int i = 0; i < car.vehicle->getNumWheels(); ++i) {
        car.vehicle->updateWheelTransform(i, true);
        if (car.hasWheels && i < 4) {
            (offset * car.vehicle->getWheelTransformWS(i))
                .getOpenGLMatrix(car.wheels[i].modelMatrix.data());
        }
    }
}

void UpdateGta5Vehicles(Gta5StreamState& state) {
    for (Gta5Vehicle& car : state.vehicles) UpdateGta5Vehicle(car);
}

}  // namespace sdl3cpp::services::impl
