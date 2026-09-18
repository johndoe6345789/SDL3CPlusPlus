#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_roof.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::fs2024 {
namespace {

using sdl3cpp::services::impl::RoofShape;

constexpr float kPitchDegrees = 35.f;
constexpr float kMaxRise = 6.f;

}  // namespace

RoofShape RoofShapeOfBld(std::uint8_t roofType) {
    switch (static_cast<BldRoofType>(roofType)) {
        case BldRoofType::Hipped:
        case BldRoofType::Mansard:
        case BldRoofType::Halfhipped:
        case BldRoofType::Sidehipped:
            return RoofShape::Hipped;
        case BldRoofType::Gabled:
        case BldRoofType::Gambrel:
        case BldRoofType::Saltbox:
        case BldRoofType::Doublesaltbox:
        case BldRoofType::Quadruplesaltbox:
        case BldRoofType::Dutchgable:
            return RoofShape::Gabled;
        case BldRoofType::Pyramidal:
        case BldRoofType::Dome:
        case BldRoofType::Onion:
        case BldRoofType::Round:
        case BldRoofType::Cone:
            return RoofShape::Pyramidal;
        default:
            return RoofShape::Flat;
    }
}

float BldRoofRise(RoofShape shape, float halfWidth) {
    if (shape == RoofShape::Flat) return 0.f;
    const float pitch = std::tan(kPitchDegrees * 3.14159265f / 180.f);
    return std::min(kMaxRise, std::max(0.f, halfWidth) * pitch);
}

}  // namespace sdl3cpp::fs2024
