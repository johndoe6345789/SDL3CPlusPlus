#pragma once

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_cgl_table.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::fs2024 {

/// An FS2024 "FBsA" CGL container (fs-base-cgl/CGL/<ddd>/<kind><ddd>
/// .cgl): a 6-digit base quadkey split between the folder and file
/// name, a small uncompressed header, an LZMA data header listing
/// every tile, then one stream per tile. Only the table is read here;
/// tiles (a London building file alone is 130 MB) load on demand.
struct CglContainer {
    std::string path;
    std::uint8_t dataProps = 0;  ///< LZMA lc/lp/pb byte of the tiles
    std::vector<CglTileEntry> tiles;  ///< sorted by `key`
};

CglContainer ReadCglContainer(const std::string& path);

/// The tile whose key is `key`, or nullptr when the container has no
/// object there (open sea, or no buildings at all in that cell).
const CglTileEntry* FindCglTile(const CglContainer& container,
                                std::uint32_t key);

/// One tile's decompressed bytes.
std::vector<std::uint8_t> ReadCglTile(const CglContainer& container,
                                      const CglTileEntry& tile);

}  // namespace sdl3cpp::fs2024
