#pragma once

#include "services/interfaces/workflow/racer/data/racer_big_endian.hpp"

#include <glm/glm.hpp>

#include <cstdint>

namespace sdl3cpp::services::impl::racer_model_detail {

// Node kinds, from the flags word at +0x00 of every node.
constexpr std::uint32_t kMeshGroup = 0x3064;
constexpr std::uint32_t kBasic = 0x5064;
constexpr std::uint32_t kSelector = 0x5065;
constexpr std::uint32_t kLodSelector = 0x5066;
constexpr std::uint32_t kTransformed = 0xD064;
constexpr std::uint32_t kTransformedPivot = 0xD065;
constexpr std::uint32_t kTransformedComputed = 0xD066;

/// Node header size, child count and child pointer offsets.
constexpr std::uint32_t kNodeHeaderBytes = 0x1C;

/// A transform node's 3x4 matrix: right, forward, up, then offset.
glm::mat4 ReadNodeTransform(const RacerBigEndianReader& r, std::uint32_t at);

/// Whether a selector node shows child `child` (-1 all, -2 none).
bool VisitNodeChild(const RacerBigEndianReader& r, std::uint32_t node,
                    std::uint32_t flags, int child);

/// A pod part's runtime scale: uniform, well below one, no offset.
bool IsPodPartScale(const glm::mat4& m);

}  // namespace sdl3cpp::services::impl::racer_model_detail
