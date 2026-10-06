#pragma once

#include "services/interfaces/workflow/racer/audio/racer_audio_clip.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <array>
#include <filesystem>

namespace sdl3cpp::services::impl {

/// The race's sound: planet music, the player's engine note (pitched by
/// speed and boost), and one-shots for the countdown, the start, wall
/// scrapes and engine fires. All from the install's own WAVs.
class RacerAudioMixer {
public:
    RacerAudioMixer() = default;
    RacerAudioMixer(const RacerAudioMixer&) = delete;
    RacerAudioMixer& operator=(const RacerAudioMixer&) = delete;
    ~RacerAudioMixer();

    /// Opens the streams. False (and silent) without an audio device.
    bool Open(const std::filesystem::path& racerDir,
              const std::string& planet);
    void Update(const RacerWorldState& state);
    bool IsOpen() const { return open_; }
    int LoadedClips() const {
        int loaded = 0;
        for (const auto& clip : clips_) loaded += clip.Loaded() ? 1 : 0;
        return loaded;
    }

private:
    enum Voice { kMusic, kEngine, kBeep, kGo, kScrape, kFire, kVoices };

    std::array<RacerAudioClip, kVoices> clips_{};
    std::array<SDL_AudioStream*, kVoices> streams_{};
    int lastCountdown_ = -1;
    bool wasRacing_ = false;
    bool wasBlocked_ = false;
    bool wasOnFire_ = false;
    bool open_ = false;
};

}  // namespace sdl3cpp::services::impl
