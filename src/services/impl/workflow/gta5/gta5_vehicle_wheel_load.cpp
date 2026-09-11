#include "services/interfaces/workflow/gta5/gta5_vehicle_load.hpp"

#include "services/interfaces/workflow/gta5/gta5_drawable_geometry.hpp"
#include "services/interfaces/workflow/gta5/gta5_mesh_transform.hpp"

namespace sdl3cpp::services::impl {
namespace {

Gta5Geometry* UploadSide(Gta5StreamState& state, const Gta5MeshData& pack,
                         const Gta5VehicleSpec& spec, bool mirror,
                         SDL_GPUDevice* device,
                         const std::shared_ptr<ILogger>& logger) {
    const std::string key =
        "wheel:" + spec.wheel + (mirror ? ":left" : ":right");
    Gta5Geometry& geometry = state.geometryCache[key];
    if (geometry.usable) return &geometry;
    const Gta5MeshData shaped =
        ShapeGta5Wheel(pack, spec.wheelRadius, spec.wheelWidth, mirror);
    // No collision: Bullet's raycast wheels are what touch the road.
    return UploadGta5MeshGeometry(state, shaped, device, geometry, false, key,
                                  logger)
               ? &geometry
               : nullptr;
}

}  // namespace

bool LoadGta5VehicleWheelMeshes(Gta5StreamState& state,
                                const Gta5VehicleSpec& spec,
                                SDL_GPUDevice* device, Gta5Vehicle& car,
                                const Gta5WheelSetup& setup,
                                const std::shared_ptr<ILogger>& logger) {
    if (!setup.hasAxles || spec.wheel.empty()) return false;
    const Gta5MeshData pack = ReadGta5ArchetypeMesh(state, Gta5Hash(spec.wheel));
    if (pack.parts.empty()) {
        if (logger) {
            logger->Warn("gta5.vehicle.spawn: no wheel '" + spec.wheel +
                         "' in the map");
        }
        return false;
    }
    Gta5Geometry* right = UploadSide(state, pack, spec, false, device, logger);
    Gta5Geometry* left = UploadSide(state, pack, spec, true, device, logger);
    if (!right || !left) return false;

    for (std::size_t i = 0; i < car.wheels.size(); ++i) {
        // Once the car faces +z, x > 0 is its left-hand side.
        Gta5Geometry* side = setup.axles[i].x() > 0.f ? left : right;
        car.wheels[i].geometry = side;
        ++side->references;
    }
    car.hasWheels = true;
    return true;
}

}  // namespace sdl3cpp::services::impl
