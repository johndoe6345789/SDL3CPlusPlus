#pragma once

#include <cstdint>

namespace sdl3cpp::services::impl {

/// A deterministic pseudo-random float in [0, 1) from a seed and up to
/// three integer coordinates -- so the same tile always places the
/// same trees, and neighbouring tiles never see the same sequence.
float Fs2024VegHash(std::uint64_t seed, int a, int b = 0, int c = 0);

}  // namespace sdl3cpp::services::impl
