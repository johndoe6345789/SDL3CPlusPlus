#include "services/interfaces/workflow/gta5/stream/gta5_world_config_load.hpp"

#include "services/interfaces/workflow/gta5/resource/gta5_json_file.hpp"

namespace sdl3cpp::services::impl {
namespace {

void ReadTileGrid(const nlohmann::json& root, Gta5WorldConfig& world) {
    const auto grid = root.find("tile_grid");
    if (grid == root.end() || !grid->is_object()) return;

    world.tileSize = grid->value("tile_size_metres", world.tileSize);

    const auto origin = grid->find("origin");
    if (origin != grid->end() && origin->is_array() && origin->size() >= 2) {
        world.gridOrigin.x = origin->at(0).get<float>();
        world.gridOrigin.y = origin->at(1).get<float>();
    }
}

void ReadLodRings(const nlohmann::json& root, Gta5WorldConfig& world) {
    const auto rings = root.find("lod_rings");
    if (rings == root.end() || !rings->is_array() || rings->empty()) return;

    world.lodRingDistances.clear();
    for (const auto& ring : *rings) {
        world.lodRingDistances.push_back(
            ring.value("max_distance_metres", -1.0f));
    }
}

}  // namespace

bool LoadGta5WorldConfig(const std::string& path, Gta5WorldConfig& world,
                         const std::shared_ptr<ILogger>& logger) {
    nlohmann::json root;
    if (!ReadGta5JsonFile(path, root, logger, "gta5 world config")) {
        // Defaults stand. Mark loaded so this is not retried every frame.
        world.loaded = true;
        return false;
    }

    ReadTileGrid(root, world);
    ReadLodRings(root, world);
    world.loaded = true;

    if (logger) {
        logger->Info("gta5 world config: tile " +
                     std::to_string(static_cast<int>(world.tileSize)) +
                     " m, " + std::to_string(world.lodRingDistances.size()) +
                     " LOD rings");
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
