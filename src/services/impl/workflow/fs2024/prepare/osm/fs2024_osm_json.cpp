#include "services/interfaces/workflow/fs2024/prepare/osm/fs2024_osm_json.hpp"

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <fstream>

namespace sdl3cpp::fs2024 {
namespace {

// A generic building with no better information: about three storeys,
// typical for the dense terraces and offices around a UK town centre.
constexpr float kDefaultBuildingHeight = 9.f;
constexpr float kMetresPerLevel = 3.f;

float ParseLeadingNumber(const std::string& text, float fallback) {
    char* end = nullptr;
    const float value = std::strtof(text.c_str(), &end);
    return end == text.c_str() ? fallback : value;
}

float BuildingHeight(const nlohmann::json& tags) {
    if (tags.contains("height")) {
        return ParseLeadingNumber(tags["height"].get<std::string>(),
                                  kDefaultBuildingHeight);
    }
    if (tags.contains("building:levels")) {
        return ParseLeadingNumber(tags["building:levels"].get<std::string>(),
                                  3.f) *
              kMetresPerLevel;
    }
    return kDefaultBuildingHeight;
}

OsmWay ReadWay(const nlohmann::json& element, bool isBuilding) {
    OsmWay way;
    const auto& tags = element.value("tags", nlohmann::json::object());
    way.name = tags.value("name", "");
    way.type = isBuilding ? tags.value("building", "yes")
                          : tags.value("highway", "");
    if (isBuilding) way.height = BuildingHeight(tags);
    for (const auto& point : element.value("geometry",
                                           nlohmann::json::array())) {
        way.points.emplace_back(point.value("lon", 0.0),
                                point.value("lat", 0.0));
    }
    return way;
}

}  // namespace

OsmData ReadOsmData(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    const auto doc = nlohmann::json::parse(in, nullptr, false);
    OsmData data;
    if (!doc.is_object()) return data;

    for (const auto& element : doc.value("elements", nlohmann::json::array())) {
        if (element.value("type", "") != "way" ||
            !element.contains("geometry")) {
            continue;
        }
        const auto& tags = element.value("tags", nlohmann::json::object());
        if (tags.contains("building")) {
            data.buildings.push_back(ReadWay(element, true));
        } else if (tags.contains("highway")) {
            data.roads.push_back(ReadWay(element, false));
        }
    }
    return data;
}

}  // namespace sdl3cpp::fs2024
