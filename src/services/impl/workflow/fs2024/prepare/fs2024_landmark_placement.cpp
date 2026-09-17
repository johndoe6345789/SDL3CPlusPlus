#include "services/interfaces/workflow/fs2024/prepare/fs2024_landmark_placement.hpp"

#include <algorithm>
#include <cctype>

namespace sdl3cpp::tools::fs2024 {
namespace {

std::string Lower(const std::string& text) {
    std::string out = text;
    for (char& c : out) c = static_cast<char>(std::tolower(c));
    return out;
}

const OsmWay* FindByName(const std::vector<OsmWay>& buildings,
                        const std::string& match) {
    const std::string needle = Lower(match);
    for (const OsmWay& building : buildings) {
        if (Lower(building.name).find(needle) != std::string::npos) {
            return &building;
        }
    }
    return nullptr;
}

}  // namespace

std::vector<LandmarkInstance> MatchLandmarks(
    const std::vector<OsmWay>& buildings,
    const std::vector<LandmarkCatalogEntry>& catalog,
    const LocalFrame& frame) {
    std::vector<LandmarkInstance> instances;
    for (const LandmarkCatalogEntry& entry : catalog) {
        const OsmWay* building = FindByName(buildings, entry.match);
        if (!building || building->points.empty()) continue;

        double lonSum = 0.0, latSum = 0.0;
        for (const auto& [lon, lat] : building->points) {
            lonSum += lon;
            latSum += lat;
        }
        const double count = static_cast<double>(building->points.size());

        LandmarkInstance instance;
        instance.model = entry.model;
        instance.headingDegrees = entry.headingDegrees;
        frame.ToEngine(lonSum / count, latSum / count, instance.x,
                      instance.z);
        instances.push_back(instance);
    }
    return instances;
}

}  // namespace sdl3cpp::tools::fs2024
