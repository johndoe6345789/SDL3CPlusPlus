#include "services/interfaces/workflow/gta5/audio/gta5_foot_phase.hpp"
#include "services/interfaces/workflow/gta5/ped/gta5_ped_gait.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPi = 3.14159265f;

}  // namespace

float Gta5FootPhase(const WorkflowContext& context, float phase,
                    float speed, float dt) {
    const float shown = context.Get<float>(kGta5WalkPhaseKey, -1.f);
    if (shown >= 0.f) return shown;
    const float stride = Gta5GaitFor(speed).stride;
    const float rate   = std::min(speed, 9.f) / stride * 2.f * kPi;
    return std::fmod(phase + rate * dt, 2.f * kPi);
}

}  // namespace sdl3cpp::services::impl
