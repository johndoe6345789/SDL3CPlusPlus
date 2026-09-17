#include "services/interfaces/workflow/fs2024/prepare/fs2024_prepare_buildings.hpp"

namespace sdl3cpp::tools::fs2024 {

std::vector<BuildingFootprint> ConvertBuildings(
    const std::vector<OsmWay>& buildings, const LocalFrame& frame) {
    std::vector<BuildingFootprint> out;
    out.reserve(buildings.size());
    for (const OsmWay& building : buildings) {
        BuildingFootprint footprint;
        footprint.height = building.height;
        footprint.footprint.reserve(building.points.size());
        for (const auto& [lon, lat] : building.points) {
            float x, z;
            frame.ToEngine(lon, lat, x, z);
            footprint.footprint.push_back({x, z});
        }
        if (footprint.footprint.size() >= 3) out.push_back(std::move(footprint));
    }
    return out;
}

}  // namespace sdl3cpp::tools::fs2024
