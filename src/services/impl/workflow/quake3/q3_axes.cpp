#include "services/interfaces/workflow/quake3/q3_axes.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace sdl3cpp::q3 {
namespace {

AxisConvention g_active = AxisConvention::EngineYUp;

bool IsZUp(AxisConvention convention) {
    return convention == AxisConvention::Ioq3ZUp;
}

}  // namespace

AxisConvention ActiveAxisConvention() {
    return g_active;
}

void SetActiveAxisConvention(AxisConvention convention) {
    g_active = convention;
}

glm::vec3 AxisUp(AxisConvention convention) {
    return IsZUp(convention) ? glm::vec3(0.0f, 0.0f, 1.0f)
                             : glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::vec3 FromQuakeDir(const glm::vec3& quake, AxisConvention convention) {
    if (IsZUp(convention)) return quake;
    return glm::vec3(quake.x, quake.z, -quake.y);
}

glm::vec3 FromQuakePoint(const glm::vec3& quake, float scale,
                         AxisConvention convention) {
    return FromQuakeDir(quake, convention) * scale;
}

glm::vec3 YawForward(float yaw, AxisConvention convention) {
    if (IsZUp(convention)) {
        // AngleVectors: yaw 0 faces +X.
        return glm::vec3(std::cos(yaw), std::sin(yaw), 0.0f);
    }
    // The Y-up engine measures yaw from -Z, the direction a camera looks
    // down by default, so this is not FromQuakeDir of the line above.
    return glm::vec3(-std::sin(yaw), 0.0f, -std::cos(yaw));
}

float YawTowards(const glm::vec3& delta, AxisConvention convention) {
    if (IsZUp(convention)) return std::atan2(delta.y, delta.x);
    // Inverting forward = (-sin, 0, -cos) needs both signs negated;
    // atan2(x, z) is the same angle turned half a circle, which is what
    // had the bots running away from whoever they were chasing.
    return std::atan2(-delta.x, -delta.z);
}

}  // namespace sdl3cpp::q3
