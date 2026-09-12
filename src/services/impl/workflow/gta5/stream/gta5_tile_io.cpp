#include "services/interfaces/workflow/gta5/stream/gta5_tile_io.hpp"

#include "services/interfaces/workflow/gta5/resource/gta5_json_file.hpp"
#include "services/interfaces/workflow/gta5/resource/gta5_placement_parse.hpp"

#include <filesystem>

namespace sdl3cpp::services::impl {

std::string Gta5TilePath(const std::string& tilesDir,
                         const Gta5TileCoord& tile) {
    return tilesDir + "/" + std::to_string(tile.x) + "_" +
           std::to_string(tile.z) + ".json";
}

bool ReadGta5TileFile(const std::string& path,
                      std::vector<Gta5Placement>& outPlacements,
                      const std::shared_ptr<ILogger>& logger) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        // Empty grid cell. Expected across most of the map.
        return false;
    }

    nlohmann::json root;
    if (!ReadGta5JsonFile(path, root, logger, "gta5.tiles")) return false;

    const auto placements = root.find("placements");
    if (placements == root.end() || !placements->is_array()) {
        if (logger) {
            logger->Warn("gta5.tiles: '" + path + "' has no placements");
        }
        return false;
    }

    outPlacements.clear();
    outPlacements.reserve(placements->size());
    for (const auto& entry : *placements) {
        Gta5Placement placement;
        if (ParseGta5Placement(entry, placement)) {
            outPlacements.push_back(std::move(placement));
        }
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
