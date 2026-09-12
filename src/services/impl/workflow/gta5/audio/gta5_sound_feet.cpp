#include "services/interfaces/workflow/gta5/audio/gta5_sound_step.hpp"

#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kStride = 1.4f;  // as the ped walk's: metres a two-step cycle

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
    const float speed = glm::length(glm::vec2(ps.velocity.x, ps.velocity.z));
    if (swimming) {
        // A stroke every 1.6 m, or slowly, treading water.
        strokePhase_ += std::max(speed / 1.6f, 0.3f) * dt;
        if (strokePhase_ >= 1.f) {
            strokePhase_ -= 1.f;
            PlayGta5Clip(playing_, sounds_.strokes, rng_, device_, spec_,
                         (speed > 0.5f ? 0.45f : 0.2f) * volume_, vary(rng_));
        }
        return;
    }
    // The walk's phase as PoseGta5Ped turns it: a foot lands as each
    // leg's swing peaks, at pi/2 and 3pi/2.
    const float before = walkPhase_;
    const float rate = std::min(speed, 6.f) / kStride * 2.f * kPi;
    walkPhase_ = std::fmod(walkPhase_ + rate * dt, 2.f * kPi);
    const auto crossed = [&](float at) {
        return walkPhase_ < before ? (before < at || walkPhase_ >= at)
                                   : (before < at && walkPhase_ >= at);
    };
    if (!ps.onGround || speed < 0.5f ||
        !(crossed(0.5f * kPi) || crossed(1.5f * kPi))) {
        return;
    }
    PlayGta5Clip(playing_, sounds_.steps, rng_, device_, spec_,
                 std::min(0.25f + speed * 0.06f, 0.6f) * volume_, vary(rng_));
}

void WorkflowGta5SoundStep::Water(WorkflowContext& context) {
    if (sounds_.water.empty()) return;
    const auto ps = context.Get<Q3PlayerState>("q3.ps", Q3PlayerState{});
    const float under = context.Get<float>("gta5.camera.underwater", -1.f);
    float surface = 0.f, gain = 0.f, ratio = 1.f;
    // Lapping within 8 m above water (GTA's y is engine -z), louder in
    // it; under it, a deep, muffled wash.
    if (under > 0.f) {
        gain = 0.6f;
        ratio = 0.55f;
    } else if (Gta5WaterHeightAt(water_, ps.origin.x, -ps.origin.z, surface)) {
        const float above = ps.origin.y + ps.mins.y - surface;
        gain = 0.35f * std::clamp(1.f - above / 8.f, 0.f, 1.f);
        if (context.GetBool("gta5.swimming", false)) gain = 0.5f;
    }
    FeedGta5Loop(waterLoop_, sounds_.water.front(), device_, spec_,
                 gain * volume_, ratio);
}

}  // namespace sdl3cpp::services::impl
