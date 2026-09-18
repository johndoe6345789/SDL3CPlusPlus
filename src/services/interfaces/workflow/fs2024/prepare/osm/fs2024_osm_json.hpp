#pragma once

#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::fs2024 {

struct OsmWay {
    std::string name;
    std::string type;  ///< the highway=* or building=* tag's value
    float height = 0.f;  ///< metres; 0 means "not given" (buildings only)
    std::vector<std::pair<double, double>> points;  ///< (lon, lat)
};

struct OsmData {
    std::vector<OsmWay> roads;
    std::vector<OsmWay> buildings;
};

/// Parses an Overpass API [out:json] response already saved to
/// `path` (fetched once with curl -- this tool makes no network
/// calls of its own, the same boundary gta5's RPF7 extraction draws).
/// A building's height comes from its own height tag if present,
/// else building:levels * 3 m, else a flat default (see the .cpp).
OsmData ReadOsmData(const std::string& path);

}  // namespace sdl3cpp::fs2024
