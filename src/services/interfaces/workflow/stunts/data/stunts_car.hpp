#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief A car's engine block, read from its .RES `simd` resource.
 *
 * The block opens with seven little-endian 16-bit fields. Gear count,
 * idle, peak-power and red-line speeds cross-check against the figures
 * the showroom text prints for every stock car, which is how the field
 * order below was established.
 */
struct StuntsEngine {
    int gears = 5;
    int performance = 0;
    int idleRpm = 800;
    int powerRpm = 4000;
    int redLine = 7000;
    int revLimit = 8000;
};

/// One car, as the game's own files describe it.
struct StuntsCar {
    std::string id;                  ///< Four-letter code, e.g. from CAR*.RES.
    std::string name;                ///< First line of the showroom text.
    std::vector<std::string> spec;   ///< The remaining showroom lines.
    StuntsEngine engine;
    bool loaded = false;
};

/// Reads one CAR*.RES image; `loaded` is false if it has no `simd`.
StuntsCar LoadStuntsCar(const std::string& path, const std::string& id);

}  // namespace sdl3cpp::services::impl
