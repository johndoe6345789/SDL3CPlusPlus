#include "services/interfaces/workflow/gta5/gta5_vehicle.hpp"

#include "services/interfaces/workflow/gta5/gta5_geometry_upload.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_body.hpp"

namespace sdl3cpp::services::impl {

bool SpawnGta5Vehicle(Gta5StreamState& state, const std::string& modelPath,
                      const glm::vec3& position, float mass,
                      SDL_GPUDevice* device, btDiscreteDynamicsWorld* world,
                      const std::shared_ptr<ILogger>& logger) {
    if (!device || !world || modelPath.empty()) return false;

    Gta5Placement placement;
    placement.archetype = "vehicle:" + modelPath;
    placement.modelPath = modelPath;

    Gta5Geometry& geometry = state.geometryCache[placement.archetype];
    if (!geometry.usable &&
        !BuildGta5Geometry(placement, device, state.textureCache, geometry,
                           logger)) {
        if (logger) {
            logger->Warn("gta5.vehicle.spawn: could not load " + modelPath);
        }
        return false;
    }

    Gta5Instance instance;
    instance.geometry = &geometry;
    instance.body = MakeGta5VehicleBody(geometry, position, mass,
                                        instance.scaledShape);
    world->addRigidBody(instance.body);
    // Held by the vehicle list, so the geometry sweep never frees the
    // mesh out from under a car that is still in the world.
    ++geometry.references;
    state.vehicles.push_back(instance);

    if (logger) {
        logger->Info("gta5.vehicle.spawn: " + modelPath + " at y=" +
                     std::to_string(position.y));
    }
    return true;
}

void UpdateGta5Vehicles(Gta5StreamState& state) {
    for (Gta5Instance& vehicle : state.vehicles) {
        if (!vehicle.body || !vehicle.body->getMotionState()) continue;
        btTransform transform;
        vehicle.body->getMotionState()->getWorldTransform(transform);
        transform.getOpenGLMatrix(vehicle.modelMatrix.data());
    }
}

}  // namespace sdl3cpp::services::impl
