#pragma once

#include <string>

namespace sdl3cpp::fs2024 {

struct PrepareArgs {
    std::string icao;      ///< airport mode when non-empty
    std::string bgl;       ///< airport BGL path (found under APPDATA if empty)
    double lat = 0.0, lon = 0.0;
    bool hasLatLon = false;
    float heading = 0.f;   ///< spawn heading; only used without --osm-json
    std::string osmJson;   ///< optional: a saved Overpass response for
                          ///< real, drivable-road spawn snapping and OSM
                          ///< box massing. Without it, --lat/--lon spawns
                          ///< exactly there and bakes no road or building
                          ///< shapes at all -- only real FS2024 landmarks
                          ///< (--landmark-catalog) and DEM terrain.
    std::string dem;
    std::string out;
    float tileSize = 1000.f;
    float extent = 4000.f;
    float spacing = 16.f;
    int texturePerTile = 512;
    /// FS2024's own `fs-base-cgl` folder. With it, every building in
    /// the bake comes from the simulator's own worldwide footprint
    /// library -- real outlines, storey counts and roof types --
    /// instead of from OSM box massing.
    std::string cglRoot;
    std::string landmarkCatalog;  ///< optional; see fs2024_landmark_catalog.hpp
    /// FS2024's own building generator texture folder
    /// (`bf-pgg/PGG/textures`). With it, walls and roofs are baked
    /// from the very assets the simulator builds its cities with.
    std::string pggTextures;
};

/// Parses argv into `PrepareArgs`. Throws std::runtime_error (with a
/// usage message) on a missing required flag or an invalid mode
/// combination -- exactly one of --icao or --lat/--lon/--osm-json.
PrepareArgs ParsePrepareArgs(int argc, char** argv);

}  // namespace sdl3cpp::fs2024
