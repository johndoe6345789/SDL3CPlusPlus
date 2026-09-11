#pragma once

#include "services/interfaces/workflow/gta5/gta5_asset_index.hpp"
#include "services/interfaces/workflow/gta5/gta5_resource.hpp"

#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <utility>

namespace sdl3cpp::services::impl {

/// The last few resources opened, by file id.
///
/// A district asks for dozens of textures and drawables from the same
/// few dictionaries in a row, and re-inflating a 30 MB .ytd for each would
/// dominate streaming. Holding a handful keeps memory bounded, which
/// keeping everything ever touched would not.
struct Gta5ResourceCache {
    std::list<std::pair<std::uint32_t, std::shared_ptr<const Gta5Resource>>>
        recent;
    /// A district pulls from dozens of dictionaries; at six the cache
    /// thrashed. The extract averages 0.8 MB per .ytd and 0.13 MB per
    /// .ydd, so this is tens of megabytes.
    std::size_t capacity{64};
};

/// The resource for `file`, loading it and evicting the least recently
/// used one when it is not already held. nullptr when it cannot be read.
std::shared_ptr<const Gta5Resource> AcquireGta5Resource(
    Gta5ResourceCache& cache, const Gta5AssetIndex& index,
    std::uint32_t file);

}  // namespace sdl3cpp::services::impl
