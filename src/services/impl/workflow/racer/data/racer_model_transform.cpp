#include "racer_model_context.hpp"

#include <cmath>

namespace sdl3cpp::services::impl::racer_model_detail {

/// Rows after the header: right, forward, up, then translation (3 floats each).
glm::mat4 ReadNodeTransform(const RacerBigEndianReader& r, std::uint32_t at) {
    glm::mat4 m(1.f);
    for (int row = 0; row < 4; ++row) {
        for (int k = 0; k < 3; ++k) {
            m[row][k] = r.F32(at + kNodeHeaderBytes + 4 * (3 * row + k));
        }
    }
    return m;
}

/// Which children to visit: all, one, or none (selector value -2).
bool VisitNodeChild(const RacerBigEndianReader& r, std::uint32_t node,
                std::uint32_t flags, int child) {
    if (flags != kSelector) return true;
    const std::int32_t selection = r.I32(node + kNodeHeaderBytes);
    if (selection == -2) return false;
    return selection < 0 || selection == child;
}

/// A pod part's runtime scale: a uniform scale well below one with no
/// translation, which the game replaces when it places the part.
bool IsPodPartScale(const glm::mat4& m) {
    const float s = std::fabs(m[0][0]);
    return s > 0.f && s < 0.05f && std::fabs(m[1][1] - m[0][0]) < 1e-4f &&
           glm::length(glm::vec3(m[3])) < 1e-3f;
}

}  // namespace sdl3cpp::services::impl::racer_model_detail
