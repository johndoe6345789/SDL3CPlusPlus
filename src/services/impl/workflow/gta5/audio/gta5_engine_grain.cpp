#include "services/interfaces/workflow/gta5/audio/gta5_engine_bank.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

/// The grain recorded nearest `revs`, 0..1 across the sweep's range.
std::size_t Nearest(const Gta5EngineSweep& sweep, float revs) {
    const auto [lo, hi] =
        std::minmax_element(sweep.revs.begin(), sweep.revs.end());
    const float want = *lo + revs * (*hi - *lo);
    std::size_t best = 0;
    for (std::size_t i = 1; i < sweep.revs.size(); ++i) {
        if (std::abs(sweep.revs[i] - want) <
            std::abs(sweep.revs[best] - want)) {
            best = i;
        }
    }
    return best;
}

}  // namespace

std::size_t NextGta5Grain(Gta5EngineVoice& voice,
                          const Gta5EngineSweep& sweep, float revs) {
    const std::size_t count = sweep.revs.size();
    if (&sweep == voice.sweep && count <= 32) return (voice.grain + 1) % count;
    const std::size_t best = count <= 32 ? 0 : Nearest(sweep, revs);
    if (&sweep != voice.sweep) return best;
    if (voice.grain + 1 < best) return std::max(voice.grain + 1, best - 8);
    if (voice.grain > best + 1) return best;
    const std::size_t low = best > 0 ? best - 1 : 0;
    const std::size_t high = std::min(best + 1, count - 1);
    return std::uniform_int_distribution<std::size_t>(low, high)(voice.rng);
}

}  // namespace sdl3cpp::services::impl
