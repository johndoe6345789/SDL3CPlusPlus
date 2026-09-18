#include "services/interfaces/workflow/fs2024/assemble/fs2024_vegetation_pick.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kForest = 2;
constexpr int kShrub = 4;

}  // namespace

std::string Fs2024VegetationBiomeFor(int landClass, double latitude) {
    const double abs_lat = std::fabs(latitude);
    if (landClass == kForest) {
        if (abs_lat >= 55.0) return "Conifer Cold PNV fallback";
        if (abs_lat >= 40.0) return "Mixed Cold PNV fallback";
        if (abs_lat >= 23.0) return "Deciduous PNV fallback";
        return "Rain PNV fallback";
    }
    if (landClass == kShrub) {
        if (abs_lat < 35.0) return "Semi-Desert PNV fallback";
        return "Scrub PNV fallback";
    }
    return {};
}

}  // namespace sdl3cpp::services::impl
