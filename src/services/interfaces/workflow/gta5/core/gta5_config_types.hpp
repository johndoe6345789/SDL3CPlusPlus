#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace sdl3cpp::services::impl {

/// Parsed config/gta5_world.json.
struct Gta5WorldConfig {
    float tileSize{512.f};
    glm::vec2 gridOrigin{-4000.f, -8000.f};
    /// Ring outer distances in metres, indexed by Gta5Lod. A negative value
    /// means unbounded.
    std::vector<float> lodRingDistances{300.f, 1000.f, 3000.f, -1.f};
    bool loaded{false};
};

/// Parsed config/streaming.json.
struct Gta5StreamingConfig {
    int loadRadiusTiles{3};
    int evictRadiusTiles{5};
    int maxResidentTiles{96};
    int maxSpawnsPerFrame{64};
    bool prefetchEnabled{true};
    float velocityLeadSeconds{2.f};
    /// Higher up, further out: a tile more per vantageMetresPerTile the
    /// player is above vantageBaseMetres, up to vantageMaxTiles.
    float vantageBaseMetres{60.f};
    float vantageMetresPerTile{60.f};
    int vantageMaxTiles{10};
    bool loaded{false};
};

}  // namespace sdl3cpp::services::impl
