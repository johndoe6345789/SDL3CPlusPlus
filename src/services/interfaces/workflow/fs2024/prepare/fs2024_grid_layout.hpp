#pragma once

namespace sdl3cpp::fs2024 {

/// The exact, key-aligned grid a bake actually lays out (see the
/// Python original this replaces, python/fs2024/tiling.py's own
/// grid_layout, for the reasoning): `requestedTileSize` need not be a
/// multiple of `spacing` -- the true pitch is whichever whole number
/// of cells comes closest, and the grid's low corner sits at an exact
/// multiple of *that*, not of the request. Skipping this step is what
/// let a ring of tiles at each bake's edge fall on a key
/// floor(x / tileSize) would compute differently for at runtime than
/// the file was named for.
struct GridLayout {
    float tileSize = 0.f;      ///< the corrected pitch
    float originX = 0.f, originZ = 0.f;  ///< exact multiple of tileSize
    float extent = 0.f;        ///< total span, both axes
    int cells = 0;             ///< heightfield side length, in points
};

GridLayout ComputeGridLayout(float extent, float requestedTileSize,
                             float spacing, int minRadiusTiles = 3);

}  // namespace sdl3cpp::fs2024
