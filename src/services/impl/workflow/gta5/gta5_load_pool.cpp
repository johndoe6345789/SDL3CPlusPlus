#include "services/interfaces/workflow/gta5/gta5_load_pool.hpp"

#include <algorithm>
#include <exception>
#include <utility>

namespace sdl3cpp::services::impl {

Gta5LoadPool::Gta5LoadPool(std::shared_ptr<const Gta5AssetIndex> index,
                           Gta5ResourceCache& resources, unsigned threads)
    : index_(std::move(index)), resources_(resources) {
    for (unsigned i = 0; i < std::max(1u, threads); ++i) {
        threads_.emplace_back([this] { Work(); });
    }
}

Gta5LoadPool::~Gta5LoadPool() {
    {
        std::lock_guard<std::mutex> hold(lock_);
        stopping_ = true;
    }
    wake_.notify_all();
    for (std::thread& thread : threads_) thread.join();
}

void Gta5LoadPool::Enqueue(std::string key, std::uint32_t hash) {
    {
        std::lock_guard<std::mutex> hold(lock_);
        jobs_.push_back({std::move(key), hash});
    }
    wake_.notify_one();
}

std::vector<Gta5PreparedGeometry> Gta5LoadPool::Take(std::size_t max) {
    std::vector<Gta5PreparedGeometry> out;
    std::lock_guard<std::mutex> hold(lock_);
    while (!done_.empty() && out.size() < max) {
        out.push_back(std::move(done_.front()));
        done_.pop_front();
    }
    return out;
}

void Gta5LoadPool::Work() {
    for (;;) {
        Job job;
        {
            std::unique_lock<std::mutex> hold(lock_);
            wake_.wait(hold, [this] { return stopping_ || !jobs_.empty(); });
            if (stopping_) return;
            job = std::move(jobs_.front());
            jobs_.pop_front();
        }
        Gta5PreparedGeometry prepared;
        prepared.key = job.key;
        try {
            prepared = PrepareGta5Geometry(*index_, resources_, claims_,
                                           job.key, job.hash);
        } catch (const std::exception&) {
            // An empty mesh reads as missing on the main thread, which is
            // better than a job that never comes back.
        }
        std::lock_guard<std::mutex> hold(lock_);
        done_.push_back(std::move(prepared));
    }
}

}  // namespace sdl3cpp::services::impl
