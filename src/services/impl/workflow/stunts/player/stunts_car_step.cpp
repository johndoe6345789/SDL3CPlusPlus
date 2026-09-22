#include "services/interfaces/workflow/stunts/player/stunts_car_physics.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

void StepStuntsCar(StuntsCarState& state, const StuntsEngine& engine,
                   const StuntsDriveTuning& tune,
                   const StuntsDriveInput& input, float dt) {
    if (dt <= 0.f) return;
    state.gear = StuntsGearFor(engine, tune, state.speed, state.gear);
    state.rpm = StuntsRpmFor(engine, tune, state.speed, state.gear);

    // Pull tails off past peak power, so the last gear runs out of
    // breath instead of accelerating as hard as the first.
    const float peak = static_cast<float>(std::max(engine.powerRpm, 1));
    const float fade = std::clamp(1.2f - state.rpm / (peak * 2.f), 0.25f, 1.f);
    const float ceiling = tune.topSpeed;
    float accel = 0.f;
    if (input.throttle > 0.f) {
        accel = input.throttle * tune.drive * fade;
    } else if (input.throttle < 0.f) {
        accel = input.throttle * tune.brake;
    }
    const float drag = state.onRoad ? tune.drag : tune.drag + tune.offRoadDrag;
    accel -= state.speed * (drag / std::max(ceiling, 1.f)) *
             std::fabs(state.speed);
    state.speed = std::clamp(state.speed + accel * dt, -ceiling * 0.25f,
                             ceiling);
    if (input.throttle == 0.f && std::fabs(state.speed) < 0.4f) {
        state.speed = 0.f;
    }

    // Arcade handling, matching the original: full steering lock is
    // available at any speed, including standstill -- a car that
    // cannot turn until it is already rolling reads as broken, not
    // as grip. It only bites a little less at speed, for stability.
    const float bite = 1.f - 0.35f * std::fabs(state.speed) / ceiling;
    state.heading += input.steer * tune.steerRate * bite * dt *
                     (state.speed < 0.f ? -1.f : 1.f);
    state.position += glm::vec3(std::cos(state.heading), 0.f,
                                std::sin(state.heading)) * state.speed * dt;
}

}  // namespace sdl3cpp::services::impl
