#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

namespace sdl3cpp::services::impl {

/// The finest streamed level -- FS2024's own building-library cut,
/// ~1.5 km at London -- and the coarsest, eight times wider, that
/// fills the distance.
constexpr int kFs2024FinestLevel = 14;
constexpr int kFs2024CoarsestLevel = 11;

/// One cell of the world grid at a quadtree level: floor(x / span),
/// floor(z / span), span being the level's tile width. The engine
/// origin sits on the coarsest level's grid, so every level's cells
/// line up with engine space and nest exactly: a cell's four children
/// are (2x..2x+1, 2z..2z+1) one level finer. Unbounded in both
/// directions.
struct Fs2024TileKey {
    int x = 0;
    int z = 0;
    int level = kFs2024FinestLevel;

    bool operator==(const Fs2024TileKey& other) const {
        return x == other.x && z == other.z && level == other.level;
    }
};

/// A tile's width at `level`, given the finest level's `tileSize`.
float Fs2024TileSpan(int level, float tileSize);

/// The key at `level` for whichever tile contains world (x, z).
Fs2024TileKey Fs2024TileKeyFor(float x, float z, float tileSize,
                               int level = kFs2024FinestLevel);

/// Engine position of the tile's north-west corner.
glm::vec3 Fs2024TileCorner(const Fs2024TileKey& key, float tileSize);

/// The tile one level coarser that holds `key`.
Fs2024TileKey Fs2024TileParent(const Fs2024TileKey& key);

/// Whether `inner` is `outer` or lies inside it (a finer level).
bool Fs2024TileContains(const Fs2024TileKey& outer,
                        const Fs2024TileKey& inner);

/// Chebyshev distance in tiles of one level: how many rings out `key`
/// sits from `centre`.
int Fs2024TileRing(const Fs2024TileKey& key, const Fs2024TileKey& centre);

/// The directory a baked tile's files live in, under `tilesRoot`.
std::string Fs2024TileDirectory(const std::string& tilesRoot,
                                const Fs2024TileKey& key);

}  // namespace sdl3cpp::services::impl

template <>
struct std::hash<sdl3cpp::services::impl::Fs2024TileKey> {
    std::size_t operator()(
        const sdl3cpp::services::impl::Fs2024TileKey& key) const noexcept {
        const auto x = static_cast<std::uint64_t>(
            static_cast<std::uint32_t>(key.x));
        const auto z = static_cast<std::uint32_t>(key.z);
        return static_cast<std::size_t>(
            (x << 32 ^ z) * 31u + static_cast<std::uint64_t>(key.level));
    }
};
