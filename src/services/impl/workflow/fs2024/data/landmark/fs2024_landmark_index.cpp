#include "services/interfaces/workflow/fs2024/data/landmark/fs2024_landmark_index.hpp"

#include "services/interfaces/workflow/fs2024/data/bgl/fs2024_scenery_objects.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <unordered_map>

namespace sdl3cpp::fs2024 {
namespace {

bool IsObjectFile(const std::filesystem::path& path) {
    std::string name = path.filename().string();
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return name.rfind("obx", 0) == 0 && path.extension() == ".bgl";
}

}  // namespace

std::vector<LandmarkPlacement> BuildLandmarkIndex(
    const std::string& libraryBgl, const std::string& sceneryRoot) {
    std::unordered_map<std::string, ModelLibraryEntry> byGuid;
    for (ModelLibraryEntry& entry : ListModelLibrary(libraryBgl)) {
        byGuid.emplace(entry.guid, std::move(entry));
    }
    std::vector<LandmarkPlacement> placements;
    namespace fs = std::filesystem;
    for (const auto& file : fs::recursive_directory_iterator(sceneryRoot)) {
        if (!file.is_regular_file() || !IsObjectFile(file.path())) continue;
        for (const auto& object : ReadSceneryObjects(file.path().string())) {
            const auto found = byGuid.find(object.guid);
            if (found == byGuid.end()) continue;
            LandmarkPlacement placement;
            placement.model = found->second;
            placement.lat = object.lat;
            placement.lon = object.lon;
            placement.headingDegrees = object.headingDegrees;
            placement.scale = object.scale > 0.f ? object.scale : 1.f;
            placements.push_back(std::move(placement));
        }
    }
    return placements;
}

}  // namespace sdl3cpp::fs2024
