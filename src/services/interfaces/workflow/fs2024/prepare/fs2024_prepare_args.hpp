#pragma once

#include <string>

namespace sdl3cpp::tools::fs2024 {

struct PrepareArgs {
    std::string icao;      ///< airport mode when non-empty
    std::string bgl;       ///< airport BGL path (found under APPDATA if empty)
    double lat = 0.0, lon = 0.0;
    bool hasLatLon = false;
    std::string osmJson;   ///< world/road mode: a saved Overpass response
    std::string dem;
    std::string out;
    float tileSize = 1000.f;
    float extent = 4000.f;
    float spacing = 16.f;
    int texturePerTile = 512;
    std::string landmarkCatalog;  ///< optional; see fs2024_landmark_catalog.hpp
    std::string wallTexture;   ///< optional; paired with roofTexture
    std::string roofTexture;   ///< optional; see fs2024_building_kit_extract.hpp
};

/// Parses argv into `PrepareArgs`. Throws std::runtime_error (with a
/// usage message) on a missing required flag or an invalid mode
/// combination -- exactly one of --icao or --lat/--lon/--osm-json.
PrepareArgs ParsePrepareArgs(int argc, char** argv);

}  // namespace sdl3cpp::tools::fs2024
