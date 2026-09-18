#pragma once

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_quadkey.hpp"
#include "services/interfaces/workflow/fs2024/data/fs2024_shared_cache.hpp"
#include "services/interfaces/workflow/fs2024/data/vec/fs2024_vec_tile.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::fs2024 {

struct CglContainer;

/// FS2024's worldwide vector layer: `fs-base-cgl/CGL/<ddd>/vec<ddd>.cgl`,
/// one container per level-6 cell holding a five-level pyramid (levels
/// 6 to 14) of the same area. Only the finest level is read. (The
/// companion `vecn` files are not geometry: their tiles are float32
/// runs -- NaNs, then values near London's 46 m geoid height -- most
/// likely heights for the same features, and unread here.) Safe to
/// share between loader threads.
class VecLibrary {
public:
    /// `cglRoot` is the folder holding `CGL/` (fs-base-cgl).
    explicit VecLibrary(std::string cglRoot);

    /// A level-14 tile's features. A missing file or an empty cell
    /// simply yields nothing.
    VecTile ReadTile(const QuadTile& tile);

private:
    std::shared_ptr<const CglContainer> Container(const std::string& baseKey);

    std::string cglRoot_;
    SharedCache<std::string, CglContainer> containers_;
};

}  // namespace sdl3cpp::fs2024
