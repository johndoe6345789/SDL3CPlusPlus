#pragma once

#include "services/interfaces/workflow/fs2024/building/fs2024_polygon.hpp"
#include "services/interfaces/workflow/fs2024/data/vec/fs2024_vec_library.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// A vector feature in its tile's own metres (x east, y south).
struct Fs2024VecShape {
    std::vector<Point2> points;
    int classBit = -1;
    std::uint8_t flags = 0;
    std::uint8_t level = 0;  ///< a bridge's height above ground

    bool Bridge() const { return (flags & 0x80) != 0; }
    bool Tunnel() const { return (flags & 0x40) != 0; }
};

struct Fs2024TileShapes {
    std::vector<Fs2024VecShape> roads;  ///< polylines, as stored
    std::vector<Fs2024VecShape> water;  ///< rings, clipped to the tile
    /// Per finest-level quad inside the tile, row-major: whether the
    /// vector layer has anything there at all. Open sea has not.
    std::vector<bool> mapped;
    int side = 0;          ///< quads per tile edge
    float quadSize = 0.f;  ///< metres

    /// Whether the vector layer maps the ground at tile-local (x, z).
    bool Maps(float x, float z) const;
};

/// FS2024's own roads and water for tile (quadX, quadY) at `level` --
/// every finest-level quad inside it -- in the tile's own metres.
/// `tileSize` is the finest level's width. Water rings are clipped to
/// the tile, since neighbours overlap by ~134 m and each would
/// otherwise draw the same water twice.
Fs2024TileShapes GatherFs2024TileShapes(sdl3cpp::fs2024::VecLibrary& vectors,
                                        int quadX, int quadY, int level,
                                        float tileSize);

/// `ring` clipped to the square [0, size] x [0, size]
/// (Sutherland-Hodgman: exact for any simple ring, convex or not).
std::vector<Point2> ClipRingToSquare(const std::vector<Point2>& ring,
                                     float size);

}  // namespace sdl3cpp::services::impl
