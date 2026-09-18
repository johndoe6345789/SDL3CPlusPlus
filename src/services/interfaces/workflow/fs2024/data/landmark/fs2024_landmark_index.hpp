#pragma once

#include "services/interfaces/workflow/fs2024/data/bgl/fs2024_bgl_model_library.hpp"

#include <string>
#include <vector>

namespace sdl3cpp::fs2024 {

/// One landmark as FS2024 itself places it: which model, and where.
struct LandmarkPlacement {
    ModelLibraryEntry model;  ///< in the landmark library BGL
    double lat = 0.0, lon = 0.0;
    float headingDegrees = 0.f;
    float scale = 1.f;
};

/// Every placement of a model from `libraryBgl` (FS2024's own landmark
/// library, Asobo_POI.BGL) found in the scenery-object sections of the
/// OBX*.bgl files under `sceneryRoot` (fs-base-genericairports/scenery,
/// the worldwide object grid). These are the game's own positions --
/// nothing is hand-placed. Placements of models the library does not
/// hold (ordinary props) are skipped.
std::vector<LandmarkPlacement> BuildLandmarkIndex(
    const std::string& libraryBgl, const std::string& sceneryRoot);

}  // namespace sdl3cpp::fs2024
