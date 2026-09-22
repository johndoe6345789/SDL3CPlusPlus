#include "services/interfaces/workflow/stunts/render/stunts_dashboard_mesh.hpp"

#include "services/impl/workflow/stunts/render/stunts_dashboard_primitives.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kTickCount = 9;
constexpr float kGaugeStartAngle = 3.92699082f;   // 225 degrees
constexpr float kGaugeSweep = -4.71238898f;       // 270 degrees, clockwise

/// One tick mark, a short radial sliver at `frac` around the sweep.
void AppendTick(GeometryPlaneMesh& mesh, glm::vec2 centre, float radius,
                float frac, bool redline) {
    const float angle = kGaugeStartAngle + kGaugeSweep * frac;
    const glm::vec2 dir(std::cos(angle), std::sin(angle));
    const glm::vec2 across(-dir.y, dir.x);
    const float half = radius * 0.03f;
    const glm::vec2 inner = centre + dir * (radius * 0.78f);
    const glm::vec2 outer = centre + dir * radius;
    AppendStuntsDashboardQuad(
        mesh, inner - across * half, inner + across * half,
        outer + across * half, outer - across * half,
        StuntsDashboardPaletteU(redline ? 86 : 15));
}

/// The needle: a thin quad from the hub to near the rim at `frac`.
void AppendNeedle(GeometryPlaneMesh& mesh, glm::vec2 centre, float radius,
                  float frac) {
    frac = std::clamp(frac, 0.f, 1.f);
    const float angle = kGaugeStartAngle + kGaugeSweep * frac;
    const glm::vec2 dir(std::cos(angle), std::sin(angle));
    const glm::vec2 across(-dir.y, dir.x);
    const float half = radius * 0.02f;
    const glm::vec2 tip = centre + dir * (radius * 0.72f);
    AppendStuntsDashboardQuad(mesh, centre - across * half,
                             centre + across * half,
                             tip + across * half * 0.2f,
                             tip - across * half * 0.2f,
                             StuntsDashboardPaletteU(15));
}

void AppendGauge(GeometryPlaneMesh& mesh, glm::vec2 centre, float radius,
                 float valueFrac) {
    AppendStuntsDashboardDisc(mesh, centre, radius, radius, 39);
    for (int i = 0; i < kTickCount; ++i) {
        const float frac = static_cast<float>(i) /
                          static_cast<float>(kTickCount - 1);
        AppendTick(mesh, centre, radius, frac, frac > 0.82f);
    }
    AppendNeedle(mesh, centre, radius, valueFrac);
}

}  // namespace

GeometryPlaneMesh BuildStuntsDashboardMesh(float speedFrac, float rpmFrac) {
    GeometryPlaneMesh mesh;
    AppendStuntsWheelRim(mesh, {0.f, -0.72f}, 0.30f, 0.40f, 39);
    AppendGauge(mesh, {-0.62f, -0.80f}, 0.16f, speedFrac);
    AppendGauge(mesh, {0.62f, -0.80f}, 0.16f, rpmFrac);
    return mesh;
}

}  // namespace sdl3cpp::services::impl
