#pragma once

#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_key.hpp"

namespace sdl3cpp::services::impl {

/// The quadtree level every streamed tile is cut at: level 14, the very
/// cut FS2024 stores its own building library at, ~1.5 km at London.
constexpr int kFs2024TileLevel = 14;

/// Where engine space sits on the Earth. Engine metres are Web Mercator
/// scaled by the ground scale at the origin (x east, z south), and
/// (0, 0) is the north-west corner of one level-14 quad tile -- so the
/// engine's own tile grid, floor(x / TileSize()), lands exactly on
/// FS2024's quadtree and key (kx, kz) is quad tile
/// (tileX + kx, tileY + kz). The scale is exact at the origin's latitude
/// and drifts by cos(lat) away from it: 0.5% at 40 km north of London,
/// which is why the origin is re-based as the player travels.
struct Fs2024GeoOrigin {
    int tileX = 0, tileY = 0;
    double metresPerUnit = 0.0;  ///< engine metres per whole-world Mercator

    float TileSize() const {
        return static_cast<float>(metresPerUnit / (1 << kFs2024TileLevel));
    }
};

/// The origin whose tile (0, 0) contains (lat, lon).
Fs2024GeoOrigin MakeFs2024GeoOrigin(double lat, double lon);

void Fs2024EngineOfLatLon(const Fs2024GeoOrigin& origin, double lat,
                          double lon, float& x, float& z);
void Fs2024LatLonOfEngine(const Fs2024GeoOrigin& origin, float x, float z,
                          double& lat, double& lon);

/// Web Mercator (0..1 across the world, v south) of engine (x, z).
void Fs2024MercatorOfEngine(const Fs2024GeoOrigin& origin, float x, float z,
                            double& u, double& v);

/// The level-14 quad tile a streaming key names.
void Fs2024QuadOfKey(const Fs2024GeoOrigin& origin, const Fs2024TileKey& key,
                     int& quadX, int& quadY);

}  // namespace sdl3cpp::services::impl
