#include "services/interfaces/workflow/gta5/audio/gta5_sound_step.hpp"

#include "services/interfaces/workflow/quake3/pmove/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPi     = 3.14159265f;
constexpr float kStride = 1.4f;  // the ped walk's metres a two-step cycle

}  // namespace

void WorkflowGta5SoundStep::Feet(WorkflowContext& context, float dt) {
    const auto ps = context.Get<Q3PlayerState>("q3.ps", Q3PlayerState{});
    const bool swimming = context.GetBool("gta5.swimming", false);
    std::uniform_real_distribution<float> vary(0.9f, 1.1f);
    if (swimming && !wasSwimming_) {
        PlayGta5Clip(playing_, sounds_.splash, rng_, device_, spec_,
                     0.7f * volume_, vary(rng_));
    }
    wasSwimming_ = swimming;
    if (state_->seated >= 0) return;
    const float speed =
        glm::length(glm::vec2(ps.velocity.x, ps.velocity.z));
    if (swimming) {
        Strokes(speed, dt);
        return;
    }
    // The walk's phase as PoseGta5Ped turns it: a foot lands as each
    // leg's swing peaks, at pi/2 and 3pi/2.
    const float before = walkPhase_;
    const float rate   = std::min(speed, 6.f) / kStride * 2.f * kPi;
    walkPhase_         = std::fmod(walkPhase_ + rate * dt, 2.f * kPi);
    const auto crossed = [&](float at) {
        return walkPhase_ < before ? (before < at || walkPhase_ >= at)
                                   : (before < at && walkPhase_ >= at);
    };
    if (!ps.onGround || speed < 0.5f ||
        !(crossed(0.5f * kPi) || crossed(1.5f * kPi))) {
        return;
    }
    // Feet under water shallower than a swim: GTA's wet steps.
    float surface = 0.f;
    const bool wading =
        Gta5WaterHeightAt(water_, ps.origin.x, -ps.origin.z, surface) &&
        ps.origin.y + ps.mins.y < surface;
    const auto& set  = wading ? sounds_.wetSteps : sounds_.steps;
    const float gain = std::min(0.25f + speed * 0.06f, 0.6f);
    PlayGta5Clip(playing_, set, rng_, device_, spec_, gain * volume_,
                 vary(rng_));
}

}  // namespace sdl3cpp::services::impl
