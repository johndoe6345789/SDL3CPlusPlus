#include "services/interfaces/workflow/fs2024/data/landmark/fs2024_landmark_catalog.hpp"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace sdl3cpp::fs2024 {

std::vector<LandmarkCatalogEntry> ReadLandmarkCatalog(
    const std::string& path) {
    std::vector<LandmarkCatalogEntry> entries;
    if (path.empty() || !std::filesystem::exists(path)) return entries;

    std::ifstream in(path, std::ios::binary);
    const auto doc = nlohmann::json::parse(in, nullptr, false);
    if (!doc.is_object()) return entries;

    for (const auto& item :
        doc.value("landmarks", nlohmann::json::array())) {
        LandmarkCatalogEntry entry;
        entry.match = item.value("match", "");
        entry.hasLatLon = item.contains("lat") && item.contains("lon");
        entry.lat = item.value("lat", 0.0);
        entry.lon = item.value("lon", 0.0);
        entry.bglPath = item.value("bgl", "");
        entry.texturesDir = item.value("texturesDir", "");
        entry.model = item.value("model", "");
        entry.headingDegrees = item.value("headingDegrees", 0.f);
        if (!entry.model.empty() && (entry.hasLatLon || !entry.match.empty())) {
            entries.push_back(std::move(entry));
        }
    }
    return entries;
}

}  // namespace sdl3cpp::fs2024
