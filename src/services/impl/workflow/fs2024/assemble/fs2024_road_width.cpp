#include "services/interfaces/workflow/fs2024/assemble/fs2024_road_build.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

float Fs2024RoadWidth(int classBit) {
    // FS2024 names none of its road classes; these follow how each one
    // runs through central London (Whitehall and the Embankment are 9,
    // side streets 21 and 23, footways 30).
    switch (classBit) {
        case 5: return 16.f;
        case 7: return 14.f;
        case 9: return 13.f;
        case 11: case 13: return 10.f;
        case 15: case 17: case 19: return 8.f;
        case 21: return 7.f;
        case 23: return 6.f;
        case 25: case 27: case 29: return 4.f;
        case 30: return 0.f;
        default: return 6.f;
    }
}

float Fs2024BridgeDeck(const Point2& p, float ground, int level,
           const std::vector<Fs2024VecShape>& water,
           const std::vector<float>& levels) {
    float deck = ground;
    for (std::size_t i = 0; i < water.size(); ++i) {
        if (Fs2024RingContains(water[i].points, p)) {
            deck = std::max(deck, levels[i] + 5.f * std::max(level, 1));
        }
    }
    return deck;
}

}  // namespace sdl3cpp::services::impl
