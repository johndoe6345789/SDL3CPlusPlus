#include "services/interfaces/workflow/fs2024/assemble/fs2024_vegetation_hash.hpp"

namespace sdl3cpp::services::impl {
namespace {

/// splitmix64, mixed with each coordinate in turn.
std::uint64_t Mix(std::uint64_t x) {
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

}  // namespace

float Fs2024VegHash(std::uint64_t seed, int a, int b, int c) {
    std::uint64_t x = Mix(seed);
    x = Mix(x ^ static_cast<std::uint64_t>(static_cast<std::uint32_t>(a)));
    x = Mix(x ^ (static_cast<std::uint64_t>(static_cast<std::uint32_t>(b))
                << 21));
    x = Mix(x ^ (static_cast<std::uint64_t>(static_cast<std::uint32_t>(c))
                << 42));
    return static_cast<float>(x >> 40) / static_cast<float>(1u << 24);
}

}  // namespace sdl3cpp::services::impl
