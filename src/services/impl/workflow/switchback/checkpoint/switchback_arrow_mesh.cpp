#include "services/interfaces/workflow/switchback/checkpoint/switchback_arrow_mesh.hpp"

#include "services/interfaces/workflow/switchback/checkpoint/switchback_mesh_primitives.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kShaftHalf = 0.3f;
constexpr float kShaftBack = -2.2f;
constexpr float kShaftFront = 0.2f;
constexpr float kHeadWide = 0.9f;
constexpr float kHeadHigh = 0.7f;
constexpr float kTipZ = 2.3f;

const SwitchbackAxes kLocalAxes{glm::vec3(1.f, 0.f, 0.f),
                                glm::vec3(0.f, 1.f, 0.f),
                                glm::vec3(0.f, 0.f, 1.f)};

void AddHead(GeometryPlaneMesh& mesh) {
    const glm::vec3 tip(0.f, 0.f, kTipZ);
    const glm::vec3 lt(-kHeadWide, kHeadHigh, kShaftFront);
    const glm::vec3 rt(kHeadWide, kHeadHigh, kShaftFront);
    const glm::vec3 rb(kHeadWide, -kHeadHigh, kShaftFront);
    const glm::vec3 lb(-kHeadWide, -kHeadHigh, kShaftFront);
    AddSwitchbackQuad(mesh, tip, rt, lt, lt, 1.0f);
    AddSwitchbackQuad(mesh, tip, rb, rt, rt, 0.6f);
    AddSwitchbackQuad(mesh, tip, lb, rb, rb, 0.3f);
    AddSwitchbackQuad(mesh, tip, lt, lb, lb, 0.6f);
    AddSwitchbackQuad(mesh, lt, rt, rb, lb, 0.6f);
}

}  // namespace

GeometryPlaneMesh BuildSwitchbackArrowMesh() {
    GeometryPlaneMesh mesh;
    AddSwitchbackBox(
        mesh,
        glm::vec3(0.f, 0.f, (kShaftBack + kShaftFront) * 0.5f),
        kLocalAxes,
        glm::vec3(kShaftHalf, kShaftHalf,
                  (kShaftFront - kShaftBack) * 0.5f));
    AddHead(mesh);
    return mesh;
}

}  // namespace sdl3cpp::services::impl
