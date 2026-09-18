#include "services/interfaces/workflow/fs2024/world/fs2024_geo_origin.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kEquatorMetres = 40075016.686;
constexpr double kTiles = 1 << kFs2024TileLevel;

double MercatorU(double lon) { return (lon + 180.0) / 360.0; }

double MercatorV(double lat) {
    const double s = std::sin(lat * kPi / 180.0);
    return 0.5 - std::log((1.0 + s) / (1.0 - s)) / (4.0 * kPi);
}

double LatOfV(double v) {
    return std::atan(std::sinh(kPi * (1.0 - 2.0 * v))) * 180.0 / kPi;
}

}  // namespace

Fs2024GeoOrigin MakeFs2024GeoOrigin(double lat, double lon) {
    constexpr int kAlign = 1 << (kFs2024TileLevel - kFs2024CoarsestLevel);
    Fs2024GeoOrigin origin;
    origin.tileX = static_cast<int>(std::floor(MercatorU(lon) * kTiles)) &
                   ~(kAlign - 1);
    origin.tileY = static_cast<int>(std::floor(MercatorV(lat) * kTiles)) &
                   ~(kAlign - 1);
    const double centreLat = LatOfV((origin.tileY + kAlign / 2.0) / kTiles);
    origin.metresPerUnit = kEquatorMetres * std::cos(centreLat * kPi / 180.0);
    return origin;
}

void Fs2024EngineOfLatLon(const Fs2024GeoOrigin& origin, double lat,
                          double lon, float& x, float& z) {
    x = static_cast<float>((MercatorU(lon) - origin.tileX / kTiles) *
                           origin.metresPerUnit);
    z = static_cast<float>((MercatorV(lat) - origin.tileY / kTiles) *
                           origin.metresPerUnit);
}

void Fs2024MercatorOfEngine(const Fs2024GeoOrigin& origin, float x, float z,
                            double& u, double& v) {
    u = origin.tileX / kTiles + x / origin.metresPerUnit;
    v = origin.tileY / kTiles + z / origin.metresPerUnit;
}

void Fs2024LatLonOfEngine(const Fs2024GeoOrigin& origin, float x, float z,
                          double& lat, double& lon) {
    double u = 0.0, v = 0.0;
    Fs2024MercatorOfEngine(origin, x, z, u, v);
    lon = u * 360.0 - 180.0;
    lat = LatOfV(v);
}

void Fs2024QuadOfKey(const Fs2024GeoOrigin& origin, const Fs2024TileKey& key,
                     int& quadX, int& quadY) {
    const int shift = kFs2024TileLevel - key.level;
    quadX = (origin.tileX >> shift) + key.x;
    quadY = (origin.tileY >> shift) + key.z;
}

}  // namespace sdl3cpp::services::impl
