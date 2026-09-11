#include "services/interfaces/workflow/gta5/gta5_vehicle_load.hpp"

#include "services/interfaces/workflow/gta5/gta5_drawable_geometry.hpp"
#include "services/interfaces/workflow/gta5/gta5_fragment_axles.hpp"
#include "services/interfaces/workflow/gta5/gta5_mesh_transform.hpp"
#include "services/interfaces/workflow/gta5/gta5_resource_cache.hpp"

namespace sdl3cpp::services::impl {
namespace {

/// The axles live in the same .yft as the body, in its fragment.
bool ReadAxles(Gta5StreamState& state, std::uint32_t hash,
               Gta5WheelSetup& setup) {
    const auto where = state.assets->drawables.find(hash);
    if (where == state.assets->drawables.end()) return false;
    const auto yft =
        AcquireGta5Resource(state.resources, *state.assets, where->second);
    std::array<glm::vec3, 4> axles{};
    if (!yft || !ReadGta5FragmentAxles(*yft, axles)) return false;
    for (std::size_t i = 0; i < axles.size(); ++i) {
        setup.axles[i] = btVector3(axles[i].x, axles[i].y, axles[i].z);
    }
    return true;
}

}  // namespace

bool LoadGta5VehicleChassis(Gta5StreamState& state,
                            const Gta5VehicleSpec& spec,
                            SDL_GPUDevice* device, Gta5Vehicle& car,
                            Gta5WheelSetup& setup,
                            const std::shared_ptr<ILogger>& logger) {
    if (!state.assets || spec.model.empty()) return false;
    const std::uint32_t hash = Gta5Hash(spec.model);

    Gta5Geometry& geometry = state.geometryCache["vehicle:" + spec.model];
    if (!geometry.usable) {
        Gta5MeshData mesh = ReadGta5ArchetypeMesh(state, hash, spec.paint);
        TurnGta5MeshAround(mesh);
        if (!UploadGta5MeshGeometry(state, mesh, device, geometry, true,
                                    spec.model, logger)) {
            if (logger) {
                logger->Warn("gta5.vehicle.spawn: no drawable '" + spec.model +
                             "' in the map");
            }
            return false;
        }
    }
    car.instance.geometry = &geometry;

    setup.radius = spec.wheelRadius;
    setup.hasAxles = ReadAxles(state, hash, setup);
    if (!setup.hasAxles && logger) {
        logger->Warn("gta5.vehicle.spawn: '" + spec.model +
                     "' has no wheel bones; wheels laid out from its box");
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
