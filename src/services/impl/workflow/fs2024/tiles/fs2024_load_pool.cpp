#include "services/interfaces/workflow/fs2024/tiles/fs2024_load_pool.hpp"

#include <algorithm>
#include <exception>
#include <utility>

namespace sdl3cpp::services::impl {

Fs2024LoadPool::Fs2024LoadPool(Build build, unsigned threads)
    : build_(std::move(build)) {
    for (unsigned i = 0; i < std::max(threads, 1u); ++i) {
        threads_.emplace_back([this] { Work(); });
    }
}

Fs2024LoadPool::~Fs2024LoadPool() {
    {
        std::lock_guard<std::mutex> hold(lock_);
        stopping_ = true;
        jobs_.clear();
    }
    wake_.notify_all();
    for (std::thread& thread : threads_) thread.join();
}

void Fs2024LoadPool::Enqueue(const Fs2024TileKey& key) {
    {
        std::lock_guard<std::mutex> hold(lock_);
        jobs_.push_back(key);
    }
    wake_.notify_one();
}

void Fs2024LoadPool::Work() {
    for (;;) {
        std::unique_lock<std::mutex> hold(lock_);
        wake_.wait(hold, [this] { return stopping_ || !jobs_.empty(); });
        if (stopping_) return;
        const Fs2024TileKey key = jobs_.front();
        jobs_.pop_front();
        ++running_;
        hold.unlock();

        Fs2024PreparedTile tile;
        try {
            tile = build_(key);
        } catch (const std::exception& error) {
            tile.error = error.what();
        } catch (...) {
            tile.error = "unknown failure";
        }
        tile.key = key;

        hold.lock();
        done_.push_back(std::move(tile));
        if (--running_ == 0 && jobs_.empty()) idle_.notify_all();
    }
}

}  // namespace sdl3cpp::services::impl
