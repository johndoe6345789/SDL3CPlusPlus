#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

namespace sdl3cpp::services::impl {

/// One cell of bl4's placement grid: floor(x / tileSize), floor(z / tileSize).
/// The grid is a bl4x-baking convention (BL4's own World Partition cells
/// carry no grid coordinate bl4x can read back out), not something BL4
/// itself defines -- see packages/bl4/README.md.
struct Bl4TileKey {
    int x = 0;
    int z = 0;

    bool operator==(const Bl4TileKey& other) const {
        return x == other.x && z == other.z;
    }
};

/// The key for whichever tile currently contains world (x, z).
Bl4TileKey Bl4TileKeyFor(float x, float z, float tileSize);

/// Chebyshev distance in tiles: how many rings out `key` sits from
/// `centre`. A square ring rather than a circle, cheap and exactly what
/// a square evict-radius test wants.
int Bl4TileRing(const Bl4TileKey& key, const Bl4TileKey& centre);

/// The directory a tile's placements.json lives in, under `mapRoot`.
std::string Bl4TileDirectory(const std::string& mapRoot, const Bl4TileKey& key);

}  // namespace sdl3cpp::services::impl

template <>
struct std::hash<sdl3cpp::services::impl::Bl4TileKey> {
    std::size_t operator()(const sdl3cpp::services::impl::Bl4TileKey& key) const noexcept {
        return (static_cast<std::size_t>(static_cast<std::uint32_t>(key.x)) << 32) ^
              static_cast<std::uint32_t>(key.z);
    }
};
