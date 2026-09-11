#pragma once

#include "services/interfaces/workflow/gta5/gta5_prepared_geometry.hpp"

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace sdl3cpp::services::impl {

/// Worker threads that prepare archetypes off the main thread.
///
/// Reading an archetype means inflating dictionaries of up to tens of MB,
/// decoding its mesh and building a collision BVH. On the main thread
/// that held a dense district at under 6 fps for twenty seconds, and the
/// physics, capped at 1/30 s a frame, ran at a fifth of real time. Here
/// every core but one takes jobs, and the main thread takes finished
/// ones and only uploads them: SDL's GPU calls and Bullet's world stay on
/// the one thread.
class Gta5LoadPool {
public:
    Gta5LoadPool(std::shared_ptr<const Gta5AssetIndex> index,
                 Gta5ResourceCache& resources, unsigned threads);
    ~Gta5LoadPool();
    Gta5LoadPool(const Gta5LoadPool&) = delete;
    Gta5LoadPool& operator=(const Gta5LoadPool&) = delete;

    void Enqueue(std::string key, std::uint32_t hash);
    /// Finished jobs, oldest first, at most `max`.
    std::vector<Gta5PreparedGeometry> Take(std::size_t max);
    std::size_t Threads() const { return threads_.size(); }

private:
    struct Job {
        std::string key;
        std::uint32_t hash{0};
    };
    void Work();

    std::shared_ptr<const Gta5AssetIndex> index_;
    Gta5ResourceCache& resources_;
    Gta5TextureClaims claims_;
    std::mutex lock_;
    std::condition_variable wake_;
    std::deque<Job> jobs_;
    std::deque<Gta5PreparedGeometry> done_;
    bool stopping_{false};
    std::vector<std::thread> threads_;
};

}  // namespace sdl3cpp::services::impl
