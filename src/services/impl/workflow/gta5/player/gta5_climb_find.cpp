#include "services/interfaces/workflow/gta5/player/gta5_climb.hpp"
#include "services/interfaces/workflow/gta5/player/gta5_look.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr float kReach   = 0.9f;  // how far ahead a wall still counts
constexpr float kKnee    = 0.35f;
constexpr float kLowest  = 0.4f;  // under this the step-up already copes
constexpr float kHighest = 2.0f;
constexpr float kOnto    = 0.35f;  // how far past the face to land

}  // namespace

bool FindGta5Ledge(btDiscreteDynamicsWorld* world,
                   const btCollisionObject* self, const glm::vec3& feet,
                   const glm::vec3& forward, float height, Gta5Ledge& out) {
    if (!world) return false;
    const glm::vec3 up(0.f, 1.f, 0.f);
    const glm::vec3 knee = feet + up * kKnee;
    glm::vec3 face;
    if (!Gta5RayHit(world, knee, knee + forward * kReach, self, &face)) {
        return false;
    }
    // Straight down onto the top, just past the face.
    const glm::vec3 over = face + forward * kOnto;
    const glm::vec3 high(over.x, feet.y + kHighest + 0.05f, over.z);
    glm::vec3 top;
    if (!Gta5RayHit(world, high, high - up * (kHighest + 0.1f), self,
                    &top)) {
        return false;
    }
    const float rise = top.y - feet.y;
    if (rise < kLowest || rise > kHighest) return false;
    // A ceiling over the top, or the ray started inside something (a
    // wall taller than the reach), means there is no room to stand.
    const glm::vec3 base = top + up * 0.05f;
    if (Gta5RayHit(world, base, base + up * height, self, nullptr)) {
        return false;
    }
    out.top  = top;
    out.rise = rise;
    return true;
}

glm::vec3 Gta5ClimbAt(const Gta5Climb& c) {
    const float u    = c.duration > 0.f ? c.t / c.duration : 1.f;
    const float lift = glm::smoothstep(0.f, 0.6f, u);
    const float over = glm::smoothstep(0.35f, 1.f, u);
    return glm::vec3(glm::mix(c.from.x, c.to.x, over),
                     glm::mix(c.from.y, c.to.y, lift),
                     glm::mix(c.from.z, c.to.z, over));
}

}  // namespace sdl3cpp::services::impl
