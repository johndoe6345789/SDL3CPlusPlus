#include "services/interfaces/workflow/switchback/checkpoint/switchback_marquee_mesh.hpp"

#include "services/interfaces/workflow/switchback/checkpoint/switchback_mesh_primitives.hpp"

#include <cstddef>
#include <initializer_list>

namespace sdl3cpp::services::impl {
namespace {

// The banner is the finish-line checker; the posts stripe red and white.
constexpr float kBannerMaterial = 1.f;
constexpr float kHalfSpan = 14.f;
constexpr float kPostHeight = 14.f;
constexpr float kPostHalf = 0.55f;
constexpr float kBannerBottom = 10.6f;
constexpr float kBannerTop = 13.4f;
constexpr float kBannerHalfDepth = 0.35f;

void AddGantry(GeometryPlaneMesh& mesh, const glm::vec3& at,
               const SwitchbackAxes& axes) {
    const glm::vec3 postHalf(kPostHalf, kPostHeight * 0.5f, kPostHalf);
    for (float side : {-1.f, 1.f}) {
        AddSwitchbackBox(mesh,
                         at + axes.across * (side * kHalfSpan) +
                             axes.up * (kPostHeight * 0.5f),
                         axes, postHalf);
    }
    const glm::vec3 bannerHalf(kHalfSpan, (kBannerTop - kBannerBottom) * 0.5f,
                               kBannerHalfDepth);
    AddSwitchbackBox(mesh,
                     at + axes.up * ((kBannerBottom + kBannerTop) * 0.5f),
                     axes, bannerHalf, kBannerMaterial);
}

SwitchbackAxes AxesToward(const glm::vec3& from, const glm::vec3& to) {
    const glm::vec3 up(0.f, 1.f, 0.f);
    glm::vec3 along(to.x - from.x, 0.f, to.z - from.z);
    along = glm::length(along) > 1e-3f ? glm::normalize(along)
                                       : glm::vec3(0.f, 0.f, 1.f);
    return SwitchbackAxes{glm::cross(up, along), up, along};
}

}  // namespace

GeometryPlaneMesh BuildSwitchbackMarqueeMesh(
    const std::vector<glm::vec3>& points) {
    GeometryPlaneMesh mesh;
    for (std::size_t i = 0; i < points.size(); ++i) {
        const glm::vec3& next = points[(i + 1) % points.size()];
        AddGantry(mesh, points[i], AxesToward(points[i], next));
    }
    return mesh;
}

}  // namespace sdl3cpp::services::impl
