#pragma once

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_tile.hpp"
#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_quadkey.hpp"
#include "services/interfaces/workflow/fs2024/data/fs2024_shared_cache.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::fs2024 {

struct CglContainer;

/// FS2024's worldwide building library: `fs-base-cgl/CGL/<ddd>/
/// bld{n,o}<ddd>.cgl`, one container per level-6 cell, each holding
/// thousands of level-14 tiles. `bldo` holds the surveyed footprints
/// (with storey counts); `bldn` holds the ones derived from imagery
/// (with a sampled roof colour but no storeys). Containers are opened
/// once and kept -- a single London file is 130 MB of index and data.
/// Safe to share between loader threads.
class BldLibrary {
public:
    /// `cglRoot` is the folder holding `CGL/` (fs-base-cgl).
    explicit BldLibrary(std::string cglRoot);
    ~BldLibrary();

    /// The tiles of both kinds covering that tile position, surveyed
    /// first. Missing files and empty cells simply yield nothing.
    std::vector<BldTile> ReadTile(const QuadTile& tile);

private:
    std::shared_ptr<const CglContainer> Container(const std::string& baseKey,
                                                  const std::string& kind);

    std::string cglRoot_;
    SharedCache<std::string, CglContainer> containers_;
};

}  // namespace sdl3cpp::fs2024
