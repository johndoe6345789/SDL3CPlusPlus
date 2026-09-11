#pragma once

#include "services/interfaces/workflow/gta5/gta5_asset_index.hpp"
#include "services/interfaces/workflow/gta5/gta5_resource.hpp"

#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <utility>

namespace sdl3cpp::services::impl {

/// Resources opened lately, by file id, up to a byte budget.
///
/// A district asks a few hundred dictionaries and drawables for many
/// textures and meshes each, and re-inflating one just dropped is the
/// cost this avoids. It is bounded by the inflated bytes held rather than
/// a count, because a .ytd runs from a few KB to tens of MB.
/// gta5.assets.index sets the budget from resource_cache_mb. The load
/// pool's workers share it, hence the lock.
struct Gta5ResourceCache {
    std::list<std::pair<std::uint32_t, std::shared_ptr<const Gta5Resource>>>
        recent;
    std::uint64_t bytes{0};
    std::uint64_t budget{std::uint64_t{4096} << 20};  // 4 GB
    std::mutex lock;
};

/// The resource for `file`, loading it when not already held and then
/// evicting the least recently used until back under budget -- never the
/// one just loaded, however large. Thread-safe; the file is read and
/// inflated outside the lock. nullptr when it cannot be read.
std::shared_ptr<const Gta5Resource> AcquireGta5Resource(
    Gta5ResourceCache& cache, const Gta5AssetIndex& index,
    std::uint32_t file);

}  // namespace sdl3cpp::services::impl
