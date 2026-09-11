#include "services/interfaces/workflow/gta5/gta5_resource_cache.hpp"

namespace sdl3cpp::services::impl {
namespace {

/// The held resource for `file`, moved to the front; lock held by caller.
std::shared_ptr<const Gta5Resource> Held(Gta5ResourceCache& cache,
                                         std::uint32_t file) {
    for (auto it = cache.recent.begin(); it != cache.recent.end(); ++it) {
        if (it->first == file) {
            cache.recent.splice(cache.recent.begin(), cache.recent, it);
            return cache.recent.front().second;
        }
    }
    return nullptr;
}

}  // namespace

std::shared_ptr<const Gta5Resource> AcquireGta5Resource(
    Gta5ResourceCache& cache, const Gta5AssetIndex& index,
    std::uint32_t file) {
    {
        std::lock_guard<std::mutex> hold(cache.lock);
        if (auto held = Held(cache, file)) return held;
    }
    if (file >= index.files.size()) return nullptr;

    // Read and inflated outside the lock, so workers do not queue on the
    // disk behind each other.
    auto resource = std::make_shared<Gta5Resource>();
    if (!LoadGta5Resource(index.files[file], *resource)) return nullptr;

    std::lock_guard<std::mutex> hold(cache.lock);
    // Another worker may have loaded the same file meanwhile.
    if (auto held = Held(cache, file)) return held;
    cache.bytes += resource->data.size();
    cache.recent.emplace_front(file, std::move(resource));
    // Callers hold a shared_ptr, so evicting one still in use is safe.
    while (cache.bytes > cache.budget && cache.recent.size() > 1) {
        cache.bytes -= cache.recent.back().second->data.size();
        cache.recent.pop_back();
    }
    return cache.recent.front().second;
}

}  // namespace sdl3cpp::services::impl
