#pragma once

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_cgl_container.hpp"
#include "services/interfaces/workflow/fs2024/data/fs2024_shared_cache.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// One of FS2024's raster layers stored as a quadtree pyramid -- `dem`
/// (levels 6-10) or `lcg` (levels 8-12) -- one file per level-6 cell
/// (`CGL/<3 digits>/<kind><3 digits>.cgl`), each tile keyed inside it as
/// `(level - 6) << 12 | index`, index being the quad digits below the
/// file's own six read as a base-4 number. Containers are opened once
/// and kept; a missing file (open ocean) is remembered as missing. Safe
/// to share between loader threads.
class Fs2024CglPyramid {
public:
    Fs2024CglPyramid(std::string cglRoot, std::string kind);

    /// Quad tile (x, y) at `level`, decompressed; empty when the layer
    /// has no tile there.
    std::vector<std::uint8_t> Read(int level, int x, int y);

private:
    std::shared_ptr<const sdl3cpp::fs2024::CglContainer> Container(
        const std::string& base);

    std::string cglRoot_, kind_;
    sdl3cpp::fs2024::SharedCache<std::string, sdl3cpp::fs2024::CglContainer>
        containers_;
};

}  // namespace sdl3cpp::services::impl
