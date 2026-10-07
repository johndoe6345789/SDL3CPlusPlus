#include "services/interfaces/workflow/racer/audio/racer_audio_mixer.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
void RacerAudioMixer::Update(const RacerWorldState& state) {
    if (!open_ || paused_) return;
    const RacerRaceState& race = state.race;
    const RacerPodState& pod = state.pod;
    FeedRacerAudioLoop(streams_[kMusic], clips_[kMusic]);
    if (menu_) {
        UpdateMenu(state);
        return;
    }
    FeedRacerAudioLoop(streams_[kEngine], clips_[kEngine]);
    if (streams_[kEngine]) {
        // The engine note rises with speed, and again under boost.
        const float pace = std::clamp(
            std::fabs(pod.speed) / state.podSpec.topSpeed, 0.f, 1.5f);
        const float boost = pod.boosting ? 0.2f : 0.f;
        const float ratio = 0.55f + 0.85f * pace + boost;
        SDL_SetAudioStreamFrequencyRatio(streams_[kEngine], ratio);
    }
    const int second = static_cast<int>(std::ceil(race.countdown));
    if (race.countdown > 0.f && second != lastCountdown_) {
        PlayRacerAudioOnce(streams_[kBeep], clips_[kBeep]);
    }
    lastCountdown_ = second;
    const bool racing = race.countdown <= 0.f;
    if (racing && !wasRacing_) PlayRacerAudioOnce(streams_[kGo], clips_[kGo]);
    wasRacing_ = racing;
    if (pod.blocked && !wasBlocked_) {
        PlayRacerAudioOnce(streams_[kScrape], clips_[kScrape]);
    }
    wasBlocked_ = pod.blocked;
    const bool onFire = pod.overheatTimer > 0.f;
    if (onFire && !wasOnFire_) {
        PlayRacerAudioOnce(streams_[kFire], clips_[kFire]);
    }
    wasOnFire_ = onFire;
    const Voice hazards[] = {kBlaster, kGeyser, kRock};
    for (int i = 0; i < 3; ++i) {
        if (state.hazardSounds & (1u << i)) {
            PlayRacerAudioOnce(streams_[hazards[i]], clips_[hazards[i]]);
        }
    }
}

}  // namespace sdl3cpp::services::impl
