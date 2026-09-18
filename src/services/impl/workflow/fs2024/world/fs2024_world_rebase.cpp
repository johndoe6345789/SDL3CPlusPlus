#include "services/interfaces/workflow/fs2024/world/fs2024_world_rebase.hpp"

namespace sdl3cpp::services::impl {

bool Fs2024RebaseDue(float x, float z, float radius) {
    return x * x + z * z > radius * radius;
}

glm::vec2 RebaseFs2024Origin(Fs2024GeoOrigin& origin, float x, float z) {
    double lat = 0.0, lon = 0.0;
    Fs2024LatLonOfEngine(origin, x, z, lat, lon);
    origin = MakeFs2024GeoOrigin(lat, lon);
    glm::vec2 moved;
    Fs2024EngineOfLatLon(origin, lat, lon, moved.x, moved.y);
    return moved;
}

}  // namespace sdl3cpp::services::impl
