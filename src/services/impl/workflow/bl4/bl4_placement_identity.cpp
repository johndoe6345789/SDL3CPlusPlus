#include "services/interfaces/workflow/bl4/bl4_placement.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

void Mix(std::uint64_t& hash, std::uint64_t value) {
    hash ^= value + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2);
}

void MixFloat(std::uint64_t& hash, float value) {
    Mix(hash, static_cast<std::uint64_t>(static_cast<std::int64_t>(std::llround(value * 1000.f))));
}

}  // namespace

std::uint64_t Bl4PlacementIdentity(const Bl4Placement& placement) {
    std::uint64_t hash = 1469598103934665603ull;
    for (char c : placement.modelPath) Mix(hash, static_cast<std::uint64_t>(c));
    for (int i = 0; i < 3; ++i) MixFloat(hash, placement.position[i]);
    MixFloat(hash, placement.rotation.x);
    MixFloat(hash, placement.rotation.y);
    MixFloat(hash, placement.rotation.z);
    MixFloat(hash, placement.rotation.w);
    for (int i = 0; i < 3; ++i) MixFloat(hash, placement.scale[i]);
    return hash;
}

}  // namespace sdl3cpp::services::impl
