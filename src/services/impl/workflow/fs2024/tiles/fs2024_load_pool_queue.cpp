#include "services/interfaces/workflow/fs2024/tiles/fs2024_load_pool.hpp"

namespace sdl3cpp::services::impl {

std::vector<Fs2024PreparedTile> Fs2024LoadPool::Take(std::size_t max) {
    std::lock_guard<std::mutex> hold(lock_);
    std::vector<Fs2024PreparedTile> out;
    while (!done_.empty() && out.size() < max) {
        out.push_back(std::move(done_.front()));
        done_.pop_front();
    }
    return out;
}

std::vector<Fs2024TileKey> Fs2024LoadPool::Drop(
    const std::function<bool(const Fs2024TileKey&)>& keep) {
    std::lock_guard<std::mutex> hold(lock_);
    std::vector<Fs2024TileKey> dropped;
    std::deque<Fs2024TileKey> kept;
    for (const Fs2024TileKey& key : jobs_) {
        (keep(key) ? kept.push_back(key) : dropped.push_back(key));
    }
    jobs_.swap(kept);
    if (jobs_.empty() && running_ == 0) idle_.notify_all();
    return dropped;
}

void Fs2024LoadPool::WaitIdle() {
    std::unique_lock<std::mutex> hold(lock_);
    idle_.wait(hold, [this] { return jobs_.empty() && running_ == 0; });
}

}  // namespace sdl3cpp::services::impl
