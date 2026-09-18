#pragma once

#include "services/interfaces/workflow/fs2024/tiles/fs2024_prepared_tile.hpp"

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace sdl3cpp::services::impl {

/// Worker threads that build tiles off the main thread, as gta5's pool
/// does its archetypes: decoding FS2024's DEM, ground cover, buildings
/// and landmark models is all CPU work, and on the frame thread a
/// single city tile cost a visible hitch. The main thread takes finished
/// tiles and only uploads them, so SDL's GPU calls and Bullet's world
/// stay on one thread. A build that throws comes back with `error` set.
class Fs2024LoadPool {
public:
    using Build = std::function<Fs2024PreparedTile(const Fs2024TileKey&)>;

    Fs2024LoadPool(Build build, unsigned threads);
    ~Fs2024LoadPool();  ///< drops what is queued, joins the workers
    Fs2024LoadPool(const Fs2024LoadPool&) = delete;
    Fs2024LoadPool& operator=(const Fs2024LoadPool&) = delete;

    void Enqueue(const Fs2024TileKey& key);
    /// Finished tiles, oldest first, at most `max`.
    std::vector<Fs2024PreparedTile> Take(std::size_t max);
    /// Removes the queued, not yet started, jobs `keep` rejects, and
    /// returns their keys.
    std::vector<Fs2024TileKey> Drop(
        const std::function<bool(const Fs2024TileKey&)>& keep);
    /// Blocks until no job is queued or running.
    void WaitIdle();
    std::size_t Threads() const { return threads_.size(); }

private:
    void Work();

    Build build_;
    std::mutex lock_;
    std::condition_variable wake_, idle_;
    std::deque<Fs2024TileKey> jobs_;
    std::deque<Fs2024PreparedTile> done_;
    std::size_t running_ = 0;
    bool stopping_ = false;
    std::vector<std::thread> threads_;
};

}  // namespace sdl3cpp::services::impl
