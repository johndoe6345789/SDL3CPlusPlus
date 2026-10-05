#pragma once

#include <glm/glm.hpp>

#include <optional>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services::impl {

/// Track triangles in engine space, bucketed on a horizontal grid so a
/// pod can find the surface under it each frame.
struct RacerGround {
    float cellSize = 8.f;
    std::vector<glm::vec3> triangles;  ///< three corners per triangle
    std::unordered_map<long long, std::vector<int>> cells;
};

/// Adds one triangle (engine space) to the grid.
void AddRacerGroundTriangle(RacerGround& ground, const glm::vec3& a,
                            const glm::vec3& b, const glm::vec3& c);

/// The highest walkable surface (normal within 60 degrees of up) at
/// (x, z) that lies below `ceiling`. Empty when nothing is there.
std::optional<float> RacerGroundHeight(const RacerGround& ground, float x,
                                       float z, float ceiling);

}  // namespace sdl3cpp::services::impl
