#include "services/interfaces/workflow/gta5/gta5_vehicle_wheels.hpp"

namespace sdl3cpp::services::impl {

void AttachGta5Wheels(Gta5Vehicle& out, btDiscreteDynamicsWorld* world,
                      const btVector3& halfExtents,
                      const Gta5WheelSetup& setup) {
    btRaycastVehicle::btVehicleTuning tuning;
    tuning.m_suspensionStiffness = 20.f;
    tuning.m_suspensionCompression = 4.4f;
    tuning.m_suspensionDamping = 2.3f;
    tuning.m_maxSuspensionTravelCm = 50.f;
    // High slip: a car that slides on every corner reads as ice.
    tuning.m_frictionSlip = 3.5f;

    out.raycaster = new btDefaultVehicleRaycaster(world);
    out.vehicle = new btRaycastVehicle(tuning, out.chassis, out.raycaster);
    // A raycast vehicle never sleeps, or its wheels stop casting.
    out.chassis->setActivationState(DISABLE_DEACTIVATION);
    world->addAction(out.vehicle);
    // x right, y up, z forward, matching the engine's axes.
    out.vehicle->setCoordinateSystem(0, 1, 2);

    const btVector3 down(0.f, -1.f, 0.f);
    const btVector3 axle(-1.f, 0.f, 0.f);
    const float x = halfExtents.x() - setup.width * 0.5f;
    const float z = halfExtents.z() - setup.radius;
    const float y = -halfExtents.y() + setup.connectionHeight;

    // Front pair steers; the rear pair drives.
    for (int i = 0; i < 4; ++i) {
        const bool front = i < 2;
        const float side = (i % 2 == 0) ? 1.f : -1.f;
        const btVector3 connection(side * x, y, front ? z : -z);
        out.vehicle->addWheel(connection, down, axle, setup.suspensionRest,
                              setup.radius, tuning, front);
    }

    for (int i = 0; i < out.vehicle->getNumWheels(); ++i) {
        btWheelInfo& wheel = out.vehicle->getWheelInfo(i);
        wheel.m_suspensionStiffness = tuning.m_suspensionStiffness;
        wheel.m_wheelsDampingRelaxation = 2.3f;
        wheel.m_wheelsDampingCompression = 4.4f;
        wheel.m_frictionSlip = tuning.m_frictionSlip;
        wheel.m_rollInfluence = 0.05f;  // resists tipping in corners
    }
}

}  // namespace sdl3cpp::services::impl
