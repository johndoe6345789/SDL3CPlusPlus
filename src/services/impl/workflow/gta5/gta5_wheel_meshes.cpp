#include "services/interfaces/workflow/gta5/gta5_wheel_meshes.hpp"

#include "services/interfaces/workflow/gta5/gta5_geometry_upload.hpp"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

void LoadGta5VehicleWheels(Gta5StreamState& state, Gta5Vehicle& car,
                           const std::string& modelPath,
                           SDL_GPUDevice* device, Gta5WheelSetup& setup,
                           const std::shared_ptr<ILogger>& logger) {
    std::filesystem::path base(modelPath);
    const std::string stem = (base.parent_path() /
                              base.stem()).string();

    int found = 0;
    for (int i = 0; i < 4; ++i) {
        const std::string path = stem + "_wheel" + std::to_string(i) + ".gltf";
        if (!std::filesystem::exists(path)) continue;

        Gta5Placement placement;
        placement.archetype = "wheel:" + path;
        placement.modelPath = path;

        Gta5Geometry& geometry = state.geometryCache[placement.archetype];
        if (!geometry.usable &&
            !BuildGta5Geometry(placement, device, state.textureCache, geometry,
                               logger)) {
            continue;
        }
        car.wheels[i].geometry = &geometry;
        ++geometry.references;
        ++found;
    }

    // The converter records where the axles actually sit, which the
    // chassis bounding box cannot tell us.
    std::ifstream sidecar(stem + "_wheels.json");
    if (sidecar.is_open()) {
        try {
            nlohmann::json doc;
            sidecar >> doc;
            const auto& axles = doc.at("axles");
            for (int i = 0; i < 4 && i < static_cast<int>(axles.size()); ++i) {
                setup.axles[i] = btVector3(axles[i][0].get<float>(),
                                           axles[i][1].get<float>(),
                                           axles[i][2].get<float>());
            }
            setup.radius = doc.value("radius", setup.radius);
            setup.hasAxles = axles.size() >= 4;
        } catch (const nlohmann::json::exception&) {
            setup.hasAxles = false;
        }
    }

    car.hasWheels = found == 4;
    if (logger && car.hasWheels) {
        logger->Info("gta5.vehicle.spawn: 4 wheel meshes loaded");
    }
}

}  // namespace sdl3cpp::services::impl
