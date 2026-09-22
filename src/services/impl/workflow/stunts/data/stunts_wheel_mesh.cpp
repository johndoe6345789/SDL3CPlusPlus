#include "services/interfaces/workflow/stunts/data/stunts_wheel_mesh.hpp"

#include "services/interfaces/workflow/stunts/data/stunts_shape_mesh.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

glm::vec3 Pos(const StuntsShape& shape, std::uint8_t index) {
    const StuntsShapeVertex& v = shape.vertices[index];
    return glm::vec3(v.x, v.y, v.z) * kStuntsUnitsToMetres;
}

/// One face's disc, or nothing if its rim points are not in the Y-Z
/// plane a wheel's axle (the shape's X axis) implies.
struct WheelDisc {
    glm::vec3 centre{0.f};
    glm::vec3 up{0.f};
    glm::vec3 side{0.f};  // perpendicular to `up`, same plane
    float radius = 0.f;
    bool valid = false;
};

WheelDisc ReadDisc(const StuntsShape& shape, const std::uint8_t* three) {
    WheelDisc disc;
    const glm::vec3 centre = Pos(shape, three[0]);
    const glm::vec3 toUp = Pos(shape, three[1]) - centre;
    const glm::vec3 toRim = Pos(shape, three[2]) - centre;
    constexpr float kPlaneTolerance = 0.001f;
    if (std::fabs(toUp.x) > kPlaneTolerance ||
        std::fabs(toRim.x) > kPlaneTolerance) {
        return disc;
    }
    disc.centre = centre;
    disc.radius = glm::length(toUp);
    if (disc.radius < kPlaneTolerance) return disc;
    disc.up = toUp / disc.radius;
    // Right-angle basis in the Y-Z plane; `toRim`'s own direction only
    // fixes which way is "round", not the basis itself.
    disc.side = glm::vec3(0.f, -disc.up.z, disc.up.y);
    if (glm::dot(disc.side, toRim) < 0.f) disc.side = -disc.side;
    disc.valid = true;
    return disc;
}

void AppendDisc(GeometryPlaneMesh& mesh, const WheelDisc& disc,
                int segments) {
    if (!disc.valid || segments < 3) return;
    const auto centre = static_cast<std::uint16_t>(mesh.vertices.size());
    mesh.vertices.push_back(
        PlanePosUvVertex{disc.centre.x, disc.centre.y, disc.centre.z,
                         0.f, 0.f});
    for (int i = 0; i < segments; ++i) {
        const float angle = 6.28318531f * static_cast<float>(i) /
                            static_cast<float>(segments);
        const glm::vec3 rim = disc.centre +
                              disc.up * std::cos(angle) * disc.radius +
                              disc.side * std::sin(angle) * disc.radius;
        mesh.vertices.push_back(
            PlanePosUvVertex{rim.x, rim.y, rim.z, 0.f, 0.f});
    }
    for (int i = 0; i < segments; ++i) {
        mesh.indices.push_back(centre);
        mesh.indices.push_back(
            static_cast<std::uint16_t>(centre + 1 + i));
        mesh.indices.push_back(static_cast<std::uint16_t>(
            centre + 1 + (i + 1) % segments));
    }
}

}  // namespace

void AppendStuntsWheelMesh(GeometryPlaneMesh& mesh, const StuntsShape& shape,
                          const StuntsShapeFace& wheel, int segments) {
    if (wheel.kind != StuntsPrimitiveKind::Wheel) return;
    AppendDisc(mesh, ReadDisc(shape, wheel.payload.data()), segments);
    AppendDisc(mesh, ReadDisc(shape, wheel.payload.data() + 3), segments);
}

}  // namespace sdl3cpp::services::impl
