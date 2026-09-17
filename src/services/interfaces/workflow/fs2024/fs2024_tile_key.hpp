#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

namespace sdl3cpp::services::impl {

/// One cell of the world grid: floor(x / tileSize), floor(z / tileSize).
/// Unbounded in both directions, the same as a player could in
/// principle walk to anywhere in the world.
struct Fs2024TileKey {
    int x = 0;
    int z = 0;

    bool operator==(const Fs2024TileKey& other) const {
        return x == other.x && z == other.z;
    }
};

/// The key for whichever tile currently contains world (x, z).
Fs2024TileKey Fs2024TileKeyFor(float x, float z, float tileSize);

/// Chebyshev distance in tiles: how many rings out `key` sits from
/// `centre`. A square ring rather than a circle, cheap and exactly
/// what a square evict-radius test wants.
int Fs2024TileRing(const Fs2024TileKey& key, const Fs2024TileKey& centre);

/// The directory a tile's terrain.fst / ground.jpg / roads.json live
/// in, under `tilesRoot`.
std::string Fs2024TileDirectory(const std::string& tilesRoot,
                                const Fs2024TileKey& key);

}  // namespace sdl3cpp::services::impl

template <>
struct std::hash<sdl3cpp::services::impl::Fs2024TileKey> {
    std::size_t operator()(
        const sdl3cpp::services::impl::Fs2024TileKey& key) const noexcept {
        return (static_cast<std::size_t>(static_cast<std::uint32_t>(key.x))
                << 32) ^
              static_cast<std::uint32_t>(key.z);
    }
};
