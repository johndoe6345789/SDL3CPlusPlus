#include "services/interfaces/workflow/fs2024/prepare/landmark/fs2024_landmark_placement.hpp"

#include <algorithm>
#include <cctype>

namespace sdl3cpp::fs2024 {
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
        LandmarkInstance instance;
        instance.model = entry.model;
        instance.headingDegrees = entry.headingDegrees;

        if (entry.hasLatLon) {
            // A plain real-world coordinate, no OSM involved at all --
            // the only option when this bake has no OSM data to match
            // against in the first place.
            frame.ToEngine(entry.lon, entry.lat, instance.x, instance.z);
            instances.push_back(instance);
            continue;
        }

        const OsmWay* building = FindByName(buildings, entry.match);
        if (!building || building->points.empty()) continue;
        double lonSum = 0.0, latSum = 0.0;
        for (const auto& [lon, lat] : building->points) {
            lonSum += lon;
            latSum += lat;
        }
        const double count = static_cast<double>(building->points.size());
        frame.ToEngine(lonSum / count, latSum / count, instance.x,
                      instance.z);
        instances.push_back(instance);
    }
    return instances;
}

}  // namespace sdl3cpp::fs2024
