#include "services/interfaces/workflow/gta5/gta5_sound_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <nlohmann/json.hpp>

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

/// Where each of five gears tops out, m/s: the revs climb through a
/// gear and fall back at the change.
constexpr float kGearTop[5] = {8.f, 17.f, 27.f, 39.f, 55.f};

float Revs(float speed) {
    int gear = 0;
    while (gear < 4 && speed > kGearTop[gear]) ++gear;
    const float low = gear > 0 ? kGearTop[gear - 1] : 0.f;
    return std::clamp((speed - low) / (kGearTop[gear] - low), 0.f, 1.f);
}

}  // namespace

void WorkflowGta5SoundStep::Engine(WorkflowContext& context, float dt) {
    const int seated = state_->seated;
    if (seated >= 0) car_ = seated;  // started when first driven
    if (sounds_.engine.empty() || car_ < 0 ||
        car_ >= static_cast<int>(state_->vehicles.size()) ||
        !state_->vehicles[car_].chassis) {
        return;
    }
    const btRigidBody* chassis = state_->vehicles[car_].chassis;
    const float speed = chassis->getLinearVelocity().length();
    const auto* keys = context.TryGet<nlohmann::json>("input.keyboard.state");
    const bool throttle = seated >= 0 && (Gta5KeyDown(keys, "W") ||
                                          Gta5KeyDown(keys, "S"));
    // Idle at rest; the throttle pulls the revs ahead of the speed, and
    // they ease rather than jump.
    const float wanted =
        std::min(1.f, 0.85f * Revs(speed) + (throttle ? 0.15f : 0.f));
    revs_ += (wanted - revs_) * std::min(1.f, dt * 6.f);
    // Heard from the player: full in the seat, fading 10 m out of it.
    const auto ps = context.Get<Q3PlayerState>("q3.ps", Q3PlayerState{});
    const btVector3 at = chassis->getCenterOfMassPosition();
    const float away =
        (at - btVector3(ps.origin.x, ps.origin.y, ps.origin.z)).length();
    const float heard = seated >= 0 ? 1.f : 1.f / (1.f + away / 10.f);
    const float gain = (throttle ? 0.5f : 0.3f) * heard * volume_;
    if (engineBank_.loaded) {
        FeedGta5Engine(engineVoice_, engineBank_, device_, spec_, revs_,
                       throttle, gain);
        return;
    }
    FeedGta5Loop(engineLoop_, sounds_.engine.front(), device_, spec_, gain,
                 0.7f + 1.6f * revs_);
}

}  // namespace sdl3cpp::services::impl
