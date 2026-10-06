#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services::impl {

/// Track triangles in engine space, bucketed on a horizontal grid so a
/// pod can find the surface under it each frame.
struct RacerGround {
    float cellSize = 8.f;
    std::vector<glm::vec3> triangles;  ///< three corners per triangle
    std::vector<std::uint32_t> flags;  ///< surface flags per triangle
    std::unordered_map<long long, std::vector<int>> cells;
    std::vector<glm::vec3> walls;      ///< steeper than rideable
    std::unordered_map<long long, std::vector<int>> wallCells;
};

/// The grid cell key for (x, z); shared by the floor and wall grids.
long long RacerGroundCell(const RacerGround& ground, float x, float z);

/// Adds one triangle (engine space): to the floor grid when a pod can
/// ride on it, otherwise to the wall grid.
void AddRacerGroundTriangle(RacerGround& ground, const glm::vec3& a,
                            const glm::vec3& b, const glm::vec3& c,
                            std::uint32_t flags = 0);

/// The highest rideable surface (normal within ~75 degrees of up) at
/// (x, z) that lies below `ceiling`. Empty when nothing is there.
std::optional<float> RacerGroundHeight(const RacerGround& ground, float x,
                                       float z, float ceiling);

/// The surface flags of the floor RacerGroundHeight would find; 0 when
/// there is none.
std::uint32_t RacerGroundFlags(const RacerGround& ground, float x, float z,
                               float ceiling);

/// True when the segment `from` -> `to` passes through a wall triangle.
bool RacerWallBetween(const RacerGround& ground, const glm::vec3& from,
                      const glm::vec3& to);

/// The index of the first wall triangle the segment hits, or -1.
int RacerWallHit(const RacerGround& ground, const glm::vec3& from,
                 const glm::vec3& to);

}  // namespace sdl3cpp::services::impl
