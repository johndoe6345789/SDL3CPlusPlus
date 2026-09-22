#include "services/impl/workflow/stunts/render/stunts_dashboard_primitives.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {
constexpr int kRingSegments = 28;
}  // namespace

float StuntsDashboardPaletteU(int materialId) {
    return (static_cast<float>(materialId) + 0.5f) / 256.f;
}

void AppendStuntsDashboardQuad(GeometryPlaneMesh& mesh, glm::vec2 a,
                              glm::vec2 b, glm::vec2 c, glm::vec2 d,
                              float paletteU) {
    const auto base = static_cast<std::uint16_t>(mesh.vertices.size());
    for (const glm::vec2& p : {a, b, c, d}) {
        mesh.vertices.push_back(PlanePosUvVertex{
            p.x, p.y, kStuntsDashboardDepth, paletteU, 0.5f});
    }
    for (const std::uint16_t i : {0, 1, 2, 0, 2, 3}) {
        mesh.indices.push_back(static_cast<std::uint16_t>(base + i));
    }
}

void AppendStuntsDashboardDisc(GeometryPlaneMesh& mesh, glm::vec2 centre,
                              float rx, float ry, int materialId) {
    const float u = StuntsDashboardPaletteU(materialId);
    const auto base = static_cast<std::uint16_t>(mesh.vertices.size());
    mesh.vertices.push_back(PlanePosUvVertex{
        centre.x, centre.y, kStuntsDashboardDepth, u, 0.5f});
    for (int i = 0; i <= kRingSegments; ++i) {
        const float a = 6.28318531f * static_cast<float>(i) /
                       static_cast<float>(kRingSegments);
        const glm::vec2 p = centre + glm::vec2(std::cos(a) * rx,
                                              std::sin(a) * ry);
        mesh.vertices.push_back(
            PlanePosUvVertex{p.x, p.y, kStuntsDashboardDepth, u, 0.5f});
    }
    for (int i = 0; i < kRingSegments; ++i) {
        mesh.indices.push_back(base);
        mesh.indices.push_back(static_cast<std::uint16_t>(base + 1 + i));
        mesh.indices.push_back(static_cast<std::uint16_t>(base + 2 + i));
    }
}

void AppendStuntsWheelRim(GeometryPlaneMesh& mesh, glm::vec2 centre,
                         float rIn, float rOut, int materialId) {
    const float u = StuntsDashboardPaletteU(materialId);
    for (int i = 0; i < kRingSegments; ++i) {
        const float a0 = 6.28318531f * static_cast<float>(i) /
                        static_cast<float>(kRingSegments);
        const float a1 = 6.28318531f * static_cast<float>(i + 1) /
                        static_cast<float>(kRingSegments);
        const glm::vec2 in0 =
            centre + glm::vec2(std::cos(a0), std::sin(a0)) * rIn;
        const glm::vec2 in1 =
            centre + glm::vec2(std::cos(a1), std::sin(a1)) * rIn;
        const glm::vec2 out0 =
            centre + glm::vec2(std::cos(a0), std::sin(a0)) * rOut;
        const glm::vec2 out1 =
            centre + glm::vec2(std::cos(a1), std::sin(a1)) * rOut;
        AppendStuntsDashboardQuad(mesh, in0, out0, out1, in1, u);
    }
}

}  // namespace sdl3cpp::services::impl
