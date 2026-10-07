#include "services/interfaces/workflow/racer/audio/racer_audio_mixer.hpp"

namespace sdl3cpp::services::impl {
namespace {

bool InWattosShop(RacerPhase phase) {
    return phase == RacerPhase::Shop || phase == RacerPhase::Junkyard ||
           phase == RacerPhase::PitDroids;
}

bool Says(const std::string& notice, const char* words) {
    return notice.find(words) != std::string::npos;
}

}  // namespace

void RacerAudioMixer::Speak(const std::string& file) {
    clips_[kSpeech] = LoadRacerAudioClip(wavs_ / "22K" / "Voice" / file);
    if (!clips_[kSpeech].Loaded()) return;
    // The voice lines share one format: the stream is made once.
    if (!streams_[kSpeech]) {
        streams_[kSpeech] = OpenRacerAudioStream(clips_[kSpeech]);
    }
    PlayRacerAudioOnce(streams_[kSpeech], clips_[kSpeech]);
}

void RacerAudioMixer::UpdateSpeech(const RacerWorldState& state) {
    const RacerFlow& flow = state.flow;
    if (flow.phase != lastPhase_) {
        // Watto's lines: "Welcome to Watto's shop", "Back again, huh?",
        // "Look around! I got a lot of junk!", his pit droids, goodbye.
        if (flow.phase == RacerPhase::Shop) {
            Speak(wattoVisits_++ == 0 ? "wtui001.wav" : "wtui022.wav");
        } else if (flow.phase == RacerPhase::Junkyard) {
            Speak("wtui053.wav");
        } else if (flow.phase == RacerPhase::PitDroids) {
            Speak("wtui015.wav");
        } else if (InWattosShop(lastPhase_)) {
            Speak("wtui056.wav");
        }
    } else if (flow.notice != lastNotice_ && !flow.notice.empty()) {
        static const char* const kNoMoney[] = {"wtui007.wav", "wtui006.wav",
                                               "wtui052.wav"};
        if (Says(flow.notice, "ALREADY") || Says(flow.notice, "FULL")) {
            Speak("wtui002.wav");             // "This part no good for you"
        } else if (Says(flow.notice, "NOT ENOUGH")) {
            Speak(kNoMoney[refusals_++ % 3]); // "No money, no parts..."
        } else if (Says(flow.notice, "FITTED") ||
                   Says(flow.notice, "JOINS")) {
            Speak("wtui011.wav");             // "Mmm... Deal!"
        }
    }
    // Picking a racer: their own line, as on the original's screen.
    const bool picking = flow.phase == RacerPhase::Tournament ||
                         flow.phase == RacerPhase::FreeRace;
    const auto& racers = state.table.racers;
    if (picking && lastRacer_ >= 0 && flow.racerIndex != lastRacer_ &&
        flow.racerIndex < static_cast<int>(racers.size()) &&
        !racers[flow.racerIndex].voice.empty()) {
        Speak(racers[flow.racerIndex].voice + "ui001.wav");
    }
    lastPhase_ = flow.phase;
    lastNotice_ = flow.notice;
    lastRacer_ = flow.racerIndex;
}

}  // namespace sdl3cpp::services::impl
