#include "services/interfaces/workflow/fs2024/data/dem/fs2024_dem_calibration.hpp"

#include <algorithm>

namespace sdl3cpp::fs2024 {
namespace {

constexpr int kKnotSpacing = 1024;
constexpr int kKnots = 65;

/// Metres at raw = -32768 + 1024 * i. See the header for how these
/// were measured; the two ends are extrapolated from their neighbours.
/// Knots 6-14 (about -25 m to +115 m) come from London alone, fitted
/// on its own at 3.0 m rms, joined to the Caspian's surface at -28 m:
/// the pooled regression flattened that span into one plateau, which
/// would have put the sea itself 13 m up.
constexpr float kKnotMetres[kKnots] = {
    -762.1f, -594.6f, -427.0f, -259.4f, -83.0f, -42.2f,
    -25.3f, -20.2f, -15.2f, -10.1f, -1.2f, 27.5f,
    56.2f, 85.0f, 113.7f, 181.8f, 249.9f, 299.1f,
    352.5f, 405.9f, 469.5f, 537.4f, 584.9f, 671.9f,
    752.9f, 823.8f, 898.0f, 984.4f, 1079.9f, 1184.5f,
    1275.5f, 1385.8f, 1502.1f, 1612.8f, 1728.3f, 1846.7f,
    1979.4f, 2099.8f, 2239.9f, 2387.2f, 2536.5f, 2698.0f,
    2874.6f, 3052.1f, 3209.0f, 3371.9f, 3528.2f, 3653.5f,
    3873.9f, 4043.4f, 4241.4f, 4414.0f, 4602.3f, 4815.3f,
    5021.7f, 5232.0f, 5480.5f, 5738.9f, 6071.3f, 6454.5f,
    6887.2f, 7317.2f, 7775.1f, 8233.0f, 8690.8f,
};

}  // namespace

float Fs2024DemMetres(std::int16_t raw) {
    const float position =
        (static_cast<float>(raw) + 32768.f) / kKnotSpacing;
    const int knot = std::min(static_cast<int>(position), kKnots - 2);
    const float t = position - static_cast<float>(knot);
    return kKnotMetres[knot] + (kKnotMetres[knot + 1] - kKnotMetres[knot]) * t;
}

}  // namespace sdl3cpp::fs2024
