#pragma once

#include <glm/glm.hpp>

#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services::impl {

/// A car node of GTA's road network (paths.rpf, nodes*.ynd), in engine
/// space, with its links: links[firstLink .. firstLink + linkCount).
struct Gta5RoadNode {
    glm::vec3 at{0.f};
    std::uint32_t firstLink{0};
    std::uint8_t linkCount{0};
};

/// A link to another node, `to` (nodes.size() when it leads nowhere
/// loaded), with the lanes running towards it and back.
struct Gta5RoadLink {
    std::uint32_t to{0};
    std::uint8_t forward{0};
    std::uint8_t back{0};
};

/// The network, and its nodes by 100 m cell.
struct Gta5Roads {
    std::vector<Gta5RoadNode> nodes;
    std::vector<Gta5RoadLink> links;
    std::unordered_map<std::uint64_t, std::vector<std::uint32_t>> cells;
    bool loaded{false};
};

inline std::uint64_t Gta5RoadCell(int x, int z) {
    return (std::uint64_t(std::uint32_t(x)) << 32) | std::uint32_t(z);
}

inline int Gta5RoadCellOf(float metres) {
    return static_cast<int>(std::floor(metres / 100.f));
}

/// Every nodes*.ynd in `dir`, car nodes only -- the first
/// NodesCountVehicle of each file. Links into other files are resolved
/// once all are read.
bool LoadGta5Roads(const std::string& dir, Gta5Roads& roads);

/// A place to park: on the road nearest `near` (engine space) within
/// `reach` metres, in the right-hand lane when traffic also runs the
/// other way, facing the way its lane runs. yaw is about +y, 0 facing
/// +z (a car's forward).
struct Gta5RoadSpot {
    glm::vec3 at{0.f};
    glm::vec2 ahead{0.f, 1.f};  // the lane's direction, engine (x, z)
    float yaw{0.f};
};
bool NearestGta5Road(const Gta5Roads& roads, const glm::vec3& near,
                     float reach, Gta5RoadSpot& spot);

}  // namespace sdl3cpp::services::impl
