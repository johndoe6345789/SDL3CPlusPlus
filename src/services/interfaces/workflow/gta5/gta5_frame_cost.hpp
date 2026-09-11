#pragma once

#include <SDL3/SDL_timer.h>

#include <cstdint>

namespace sdl3cpp::services::impl {

/// CPU milliseconds the gta5 steps spent this frame, for gta5.frame.stats
/// to average and to break a hitch down with.
struct Gta5FrameCost {
    double load{0.0};    // all of gta5.tiles.load, including:
    double finish{0.0};  //   uploading what the load pool finished
    double spawn{0.0};   //   spawning instances and their bodies
    double adopt{0.0};   //   taking finished tile reads
    double evict{0.0};
    double cull{0.0};
    double draw{0.0};
    int spawned{0};

    void Add(const Gta5FrameCost& o) {
        load += o.load;
        finish += o.finish;
        spawn += o.spawn;
        adopt += o.adopt;
        evict += o.evict;
        cull += o.cull;
        draw += o.draw;
        spawned += o.spawned;
    }
};

/// Adds the milliseconds from its construction to its destruction.
class Gta5CostTimer {
public:
    explicit Gta5CostTimer(double& into)
        : into_(into), start_(SDL_GetTicksNS()) {}
    ~Gta5CostTimer() {
        into_ += static_cast<double>(SDL_GetTicksNS() - start_) / 1e6;
    }
    Gta5CostTimer(const Gta5CostTimer&) = delete;
    Gta5CostTimer& operator=(const Gta5CostTimer&) = delete;

private:
    double& into_;
    std::uint64_t start_;
};

}  // namespace sdl3cpp::services::impl
