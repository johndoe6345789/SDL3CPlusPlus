#include "services/interfaces/workflow/fs2024/data/vec/fs2024_vec_frame.hpp"

#include <cmath>

namespace sdl3cpp::fs2024 {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kEquatorMetres = 40075016.686;
constexpr double kMarginMetres = 214.4;  ///< past each edge of the tile
constexpr double kCentre = 32768.0;

/// The tile's own ground width: Web Mercator's scale at its centre.
double TileMetres(const QuadTile& tile) {
    const double side = static_cast<double>(1 << tile.level);
    const double v = (tile.y + 0.5) / side;
    const double lat = std::atan(std::sinh(kPi * (1.0 - 2.0 * v)));
    return kEquatorMetres * std::cos(lat) / side;
}

}  // namespace

void VecPointInTile(const QuadTile& tile, const VecPoint& point, double& fx,
                    double& fy) {
    const double width = TileMetres(tile);
    const double unitsPerTile = 65536.0 * width / (width + 2 * kMarginMetres);
    fx = 0.5 + (point.x - kCentre) / unitsPerTile;
    fy = 0.5 - (point.y - kCentre) / unitsPerTile;
}

}  // namespace sdl3cpp::fs2024
