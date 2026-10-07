#include "services/interfaces/workflow/racer/audio/racer_voice.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr float kAirLine = 1.2f;     // seconds aloft before "Whoa!"
constexpr float kBetweenTaunts = 7.f;

}  // namespace

void RacerVoice::Update(const RacerWorldState& state, float dt) {
    quiet_ -= dt;
    tauntQuiet_ -= dt;
    const RacerRaceState& race = state.race;
    const RacerPodState& pod = state.pod;
    const bool racing = race.countdown <= 0.f && !race.finished;
    if (racing && !wasRacing_) Say(RacerVoiceEvent::Whoop, false);
    if (race.finished && !wasFinished_) {
        Say(race.position == 1 ? RacerVoiceEvent::Win
                               : RacerVoiceEvent::Lose,
            true);
    }
    wasRacing_ = racing;
    wasFinished_ = race.finished;
    if (!racing) return;
    const bool air = pod.airTime > kAirLine;
    const bool fire = pod.overheatTimer > 0.f;
    const float damage = 0.5f * (pod.engineDamage[0] + pod.engineDamage[1]);
    if (fire && !wasFire_) Say(RacerVoiceEvent::Scream, true);
    if (air && !wasAir_) Say(RacerVoiceEvent::Air, false);
    if (pod.blocked && !wasBlocked_) Say(RacerVoiceEvent::Hit, false);
    if (pod.bumped) Say(RacerVoiceEvent::Bump, false);
    if (damage > 0.5f && lastDamage_ <= 0.5f) {
        Say(RacerVoiceEvent::Damage, false);
    }
    // Repaired after a battering: the racer is relieved.
    if (damage > 0.3f) hurt_ = true;
    if (hurt_ && damage < 0.05f) {
        hurt_ = false;
        Say(RacerVoiceEvent::Repaired, false);
    }
    // Passing someone earns them a taunt, now and then.
    if (lastPosition_ > 0 && race.position < lastPosition_ &&
        tauntQuiet_ <= 0.f) {
        Say(RacerVoiceEvent::Taunt, false);
        tauntQuiet_ = kBetweenTaunts;
    }
    lastPosition_ = race.position;
    lastDamage_ = damage;
    wasAir_ = air;
    wasFire_ = fire;
    wasBlocked_ = pod.blocked;
}

}  // namespace sdl3cpp::services::impl
