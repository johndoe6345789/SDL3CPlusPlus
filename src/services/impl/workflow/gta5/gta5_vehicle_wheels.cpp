#include "services/interfaces/workflow/gta5/gta5_vehicle_wheels.hpp"

namespace sdl3cpp::services::impl {

void AttachGta5Wheels(Gta5Vehicle& out, btDiscreteDynamicsWorld* world,
                      const btVector3& halfExtents,
                      const Gta5WheelSetup& setup) {
    btRaycastVehicle::btVehicleTuning tuning;
    // Firm: at 20 the rear sat 12 cm down under its own weight and
    // bottomed over every bump. At 50 it settles about 5 cm.
    tuning.m_suspensionStiffness = 50.f;
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
    const btVector3 axis(-1.f, 0.f, 0.f);
    const float x = halfExtents.x() - setup.width * 0.5f;
    const float z = halfExtents.z() - setup.radius;
    const float y = -halfExtents.y() + setup.connectionHeight;

    // Front pair steers; the rear pair drives. Order is front-right,
    // front-left, rear-right, rear-left, matching the converter.
    for (int i = 0; i < 4; ++i) {
        const bool front = i < 2;
        btVector3 connection(((i % 2 == 0) ? 1.f : -1.f) * x, y,
                             front ? z : -z);
        if (setup.hasAxles) {
            // The suspension connects above the axle; the wheel hangs
            // down to it by the rest length.
            connection = setup.axles[i] + btVector3(0.f, setup.suspensionRest,
                                                    0.f);
        }
        // The longer rest length hangs each wheel rideHeight below its
        // modelled axle: the body rides higher and clears the tyres.
        out.vehicle->addWheel(connection, down, axis,
                              setup.suspensionRest + setup.rideHeight,
                              setup.radius, tuning, front);
    }

    for (int i = 0; i < out.vehicle->getNumWheels(); ++i) {
        btWheelInfo& wheel = out.vehicle->getWheelInfo(i);
        wheel.m_suspensionStiffness = tuning.m_suspensionStiffness;
        // About 0.4 of critical (2 sqrt k): settles without bouncing.
        wheel.m_wheelsDampingRelaxation = 6.f;
        wheel.m_wheelsDampingCompression = 4.4f;
        wheel.m_frictionSlip = tuning.m_frictionSlip;
        wheel.m_rollInfluence = 0.05f;  // resists tipping in corners
    }
    // A new wheel has no raycast result until the first physics step, and
    // its transform is built from that result: read before then, it gave
    // a wheel height of 8.5e17. This sets each wheel at its rest length.
    out.vehicle->resetSuspension();
}

}  // namespace sdl3cpp::services::impl
