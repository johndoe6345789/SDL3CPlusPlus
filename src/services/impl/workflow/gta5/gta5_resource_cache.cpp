#include "services/interfaces/workflow/gta5/gta5_resource_cache.hpp"

namespace sdl3cpp::services::impl {

std::shared_ptr<const Gta5Resource> AcquireGta5Resource(
    Gta5ResourceCache& cache, const Gta5AssetIndex& index,
    std::uint32_t file) {
    for (auto it = cache.recent.begin(); it != cache.recent.end(); ++it) {
        if (it->first == file) {
            cache.recent.splice(cache.recent.begin(), cache.recent, it);
            return cache.recent.front().second;
        }
    }
    if (file >= index.files.size()) return nullptr;

    auto resource = std::make_shared<Gta5Resource>();
    if (!LoadGta5Resource(index.files[file], *resource)) return nullptr;
    cache.recent.emplace_front(file, std::move(resource));
    // Callers hold a shared_ptr, so evicting one still in use is safe.
    while (cache.recent.size() > cache.capacity) cache.recent.pop_back();
    return cache.recent.front().second;
}

}  // namespace sdl3cpp::services::impl
