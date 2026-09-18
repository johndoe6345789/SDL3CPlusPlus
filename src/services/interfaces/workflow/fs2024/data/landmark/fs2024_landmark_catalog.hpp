#pragma once

#include <string>
#include <vector>

namespace sdl3cpp::fs2024 {

/// One named, hand-modelled FS2024 landmark to substitute for an OSM
/// building's flat massing. `bglPath`/`texturesDir` point at the
/// user's own FS2024 install (this tool makes no attempt to locate it
/// itself, matching every other input it takes); `model` is the name
/// the model's own `GXML` chunk gives it (e.g. `WestminsterPalace`).
/// `headingDegrees` is a plain manual value, not computed from the
/// OSM footprint: a real landmark's footprint is rarely a simple
/// rectangle, so there is no reliable automatic orientation -- it is
/// tuned by eye in-game instead, the same way runway markings were.
struct LandmarkCatalogEntry {
    std::string match;  ///< case-insensitive substring of an OSM name;
                       ///< ignored when hasLatLon is set
    bool hasLatLon = false;  ///< place at lat/lon directly, no OSM needed
    double lat = 0.0, lon = 0.0;
    std::string bglPath;
    std::string texturesDir;
    std::string model;
    float headingDegrees = 0.f;
};

/// Reads a landmark catalog JSON (`{"landmarks": [...]}`). Returns an
/// empty list if `path` is empty or does not exist -- landmarks are
/// entirely optional, unlike every other `fs2024_prepare` input.
std::vector<LandmarkCatalogEntry> ReadLandmarkCatalog(
    const std::string& path);

}  // namespace sdl3cpp::fs2024
