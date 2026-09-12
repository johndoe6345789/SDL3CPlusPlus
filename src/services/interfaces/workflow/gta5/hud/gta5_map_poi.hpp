#pragma once

#include "services/interfaces/i_logger.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// Icons fill one 256-wide atlas row of 32-pixel cells.
inline constexpr std::size_t kGta5MapMaxCategories = 8;

struct Gta5MapPoiCategory {
    std::string label;  // the legend's line
    char letter{'?'};   // on the icon
    std::array<std::uint8_t, 3> colour{255, 255, 255};
};

/// A place on the map, in GTA world metres: x east, y north.
struct Gta5MapPoi {
    int category{0};
    float x{0.f};
    float y{0.f};
    std::string name;
};

struct Gta5MapPois {
    std::vector<Gta5MapPoiCategory> categories;
    std::vector<Gta5MapPoi> points;
};

/// Read points of interest from JSON: "categories", each {id, label,
/// letter, colour: [r, g, b]}, and "points", each {category (an id),
/// name, x, y}. Empty, and logged, when the file is missing or malformed.
Gta5MapPois LoadGta5MapPois(const std::string& path,
                            const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
