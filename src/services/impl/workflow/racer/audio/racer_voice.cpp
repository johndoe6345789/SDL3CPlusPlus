#include "services/interfaces/workflow/racer/audio/racer_voice.hpp"

#include <SDL3/SDL_init.h>

#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kBetweenLines = 3.f;    // seconds
constexpr float kVoiceGain = 1.f;

}  // namespace

RacerVoice::RacerVoice(std::filesystem::path racerDir, std::string voice)
    : dir_(std::move(racerDir) / "data" / "wavs" / "22K" / "Voice"),
      voice_(std::move(voice)) {}

RacerVoice::~RacerVoice() {
    if (stream_) {
        SDL_DestroyAudioStream(stream_);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
}

void RacerVoice::SetPaused(bool paused) {
    if (!stream_) return;
    if (paused) {
        SDL_PauseAudioStreamDevice(stream_);
    } else {
        SDL_ResumeAudioStreamDevice(stream_);
    }
}

void RacerVoice::Say(RacerVoiceEvent event, bool urgent) {
    if (voice_.empty() || (!urgent && quiet_ > 0.f)) return;
    const std::string file = RacerVoiceLineFile(voice_, event, said_++);
    auto found = clips_.find(file);
    if (found == clips_.end()) {
        found = clips_.emplace(file, LoadRacerAudioClip(dir_ / file)).first;
    }
    const RacerAudioClip& clip = found->second;
    if (!clip.Loaded()) return;
    if (!stream_ && SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        // Every line shares one format; the stream is made for the first.
        stream_ = OpenRacerAudioStream(clip);
        if (stream_) SDL_SetAudioStreamGain(stream_, kVoiceGain);
        if (!stream_) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
    PlayRacerAudioOnce(stream_, clip);
    quiet_ = kBetweenLines;
}

}  // namespace sdl3cpp::services::impl
