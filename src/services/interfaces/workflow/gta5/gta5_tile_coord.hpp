#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace sdl3cpp::services::impl {

/// A cell in the horizontal tile grid. Tiles are columns: they span the
/// whole vertical range, so a tunnel and the tower above it share a tile.
struct Gta5TileCoord {
    int x{0};
    int z{0};

    bool operator==(const Gta5TileCoord& other) const {
        return x == other.x && z == other.z;
    }
};

struct Gta5TileCoordHash {
    std::size_t operator()(const Gta5TileCoord& tile) const noexcept {
        // Tile indices are small and signed. Mixing them into one 64-bit
        // key avoids the clustering a plain XOR gives on a grid.
        const std::uint64_t ux = static_cast<std::uint32_t>(tile.x);
        const std::uint64_t uz = static_cast<std::uint32_t>(tile.z);
        constexpr std::uint64_t kMix = 0x9E3779B97F4A7C15ULL;
        return static_cast<std::size_t>((ux << 32) ^ (uz * kMix));
    }
};

/// Scene-object tag identifying the tile that spawned an object, so
/// eviction can find its objects again. Format: "gta5:<x>_<z>".
std::string MakeGta5TileTag(const Gta5TileCoord& tile);

}  // namespace sdl3cpp::services::impl
