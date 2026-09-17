#include "services/interfaces/workflow/fs2024/prepare/landmark/fs2024_landmark_tile_write.hpp"

#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_key.hpp"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_map>

namespace sdl3cpp::tools::fs2024 {

void WriteLandmarkInstances(const std::string& outDir,
                           const std::vector<LandmarkInstance>& instances,
                           float tileSize) {
    namespace fs = std::filesystem;
    using services::impl::Fs2024TileDirectory;
    using services::impl::Fs2024TileKey;
    using services::impl::Fs2024TileKeyFor;

    std::unordered_map<Fs2024TileKey, nlohmann::json> byTile;
    for (const LandmarkInstance& instance : instances) {
        const Fs2024TileKey key =
            Fs2024TileKeyFor(instance.x, instance.z, tileSize);
        byTile[key]["instances"].push_back(
            {{"model", instance.model},
            {"x", instance.x},
            {"z", instance.z},
            {"headingDegrees", instance.headingDegrees}});
    }
    for (const auto& [key, doc] : byTile) {
        const fs::path dir = Fs2024TileDirectory(outDir, key);
        fs::create_directories(dir);
        std::ofstream(dir / "landmarks.json") << doc.dump();
    }
}

}  // namespace sdl3cpp::tools::fs2024
