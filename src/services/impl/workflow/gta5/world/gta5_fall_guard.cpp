#include "services/interfaces/workflow/gta5/world/gta5_finite_guard.hpp"

#include "services/interfaces/workflow/gta5/player/gta5_look.hpp"
#include "services/interfaces/workflow/gta5/player/gta5_player_pin.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"

#include <string>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kFalling = -12.f;  // m/s down: a second and more of fall
constexpr float kBelow = 1000.f;   // how far down an ordinary fall lands
constexpr float kAbove = 300.f;    // how far up the lost floor may be

}  // namespace

void GuardGta5PlayerFall(Gta5StreamState& state, WorkflowContext& context,
                         btRigidBody* player,
                         const std::shared_ptr<ILogger>& logger) {
    const auto* ps = context.TryGet<Q3PlayerState>("q3.ps");
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    if (!ps || !world || !player || state.seated >= 0 ||
        ps->velocity.y > kFalling) {
        return;
    }
    const glm::vec3 at = ps->origin;
    // Something below: an ordinary fall, off a roof or a bridge.
    if (Gta5RayHit(world, at, at - glm::vec3(0.f, kBelow, 0.f), player,
                   nullptr)) {
        return;
    }
    // Nothing below, yet a floor above: the player went through it.
    glm::vec3 floor;
    if (!Gta5RayHit(world, at + glm::vec3(0.f, kAbove, 0.f), at, player,
                    &floor)) {
        return;
    }
    const glm::vec3 back = floor + glm::vec3(0.f, 1.5f, 0.f);
    if (logger) {
        logger->Warn("gta5.guard: player fell through the floor at (" +
                     std::to_string(at.x) + ", " + std::to_string(at.y) +
                     ", " + std::to_string(at.z) + "); put back on it at y " +
                     std::to_string(back.y));
    }
    PinGta5Player(context, player, back);
}

}  // namespace sdl3cpp::services::impl
