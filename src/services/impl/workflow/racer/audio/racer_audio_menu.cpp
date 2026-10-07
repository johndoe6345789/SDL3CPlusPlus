#include "services/interfaces/workflow/racer/audio/racer_audio_mixer.hpp"

namespace sdl3cpp::services::impl {

void RacerAudioMixer::UpdateMenu(const RacerWorldState& state) {
    const RacerFlow& flow = state.flow;
    UpdateSpeech(state);
    const std::array<int, 7> cursor = {
        static_cast<int>(flow.phase), flow.menuRow, flow.shopRow,
        flow.trackIndex, flow.racerIndex, flow.laps, flow.opponents};
    const bool first = menuTruguts_ < 0;
    if (!first && flow.profile.truguts < menuTruguts_) {
        PlayRacerAudioOnce(streams_[kCoin], clips_[kCoin]);
    } else if (!first && cursor[0] != menuCursor_[0]) {
        PlayRacerAudioOnce(streams_[kSelect], clips_[kSelect]);
    } else if (!first && cursor != menuCursor_) {
        PlayRacerAudioOnce(streams_[kMove], clips_[kMove]);
    }
    menuCursor_ = cursor;
    menuTruguts_ = flow.profile.truguts;
}

void RacerAudioMixer::SetPaused(bool paused) {
    if (paused == paused_) return;
    paused_ = paused;
    for (SDL_AudioStream* stream : streams_) {
        if (!stream) continue;
        if (paused) {
            SDL_PauseAudioStreamDevice(stream);
        } else {
            SDL_ResumeAudioStreamDevice(stream);
        }
    }
}

}  // namespace sdl3cpp::services::impl
