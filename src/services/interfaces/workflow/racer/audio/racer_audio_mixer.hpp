#pragma once

#include "services/interfaces/workflow/racer/audio/racer_audio_clip.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <array>
#include <filesystem>

namespace sdl3cpp::services::impl {

/// The race's sound: planet music, the player's engine note (pitched by
/// speed and boost), and one-shots for the countdown, the start, wall
/// scrapes and engine fires. In menu mode: the title music and the
/// cursor, select and coin sounds. All from the install's own WAVs.
class RacerAudioMixer {
public:
    RacerAudioMixer() = default;
    RacerAudioMixer(const RacerAudioMixer&) = delete;
    RacerAudioMixer& operator=(const RacerAudioMixer&) = delete;
    ~RacerAudioMixer();

    /// Opens the streams. False (and silent) without an audio device.
    /// `menu` opens the title screens' sounds instead of the race's.
    bool Open(const std::filesystem::path& racerDir,
              const std::string& planet, bool menu = false);
    void Update(const RacerWorldState& state);
    /// Freezes every stream (pause) or lets them run on.
    void SetPaused(bool paused);
    bool IsOpen() const { return open_; }
    int LoadedClips() const {
        int loaded = 0;
        for (const auto& clip : clips_) loaded += clip.Loaded() ? 1 : 0;
        return loaded;
    }

private:
    enum Voice {
        kMusic, kEngine, kBeep, kGo, kScrape, kFire,
        kBlaster, kGeyser, kRock, kSpeech, kVoices
    };
    // Menu mode reuses the one-shot voices for its own sounds.
    static constexpr int kMove = kBeep;
    static constexpr int kSelect = kGo;
    static constexpr int kCoin = kScrape;

    void UpdateMenu(const RacerWorldState& state);
    /// Watto in his shop, and a racer's line when picked.
    void UpdateSpeech(const RacerWorldState& state);
    /// Plays a line from data/wavs/22K/Voice on the speech voice.
    void Speak(const std::string& file);

    std::array<RacerAudioClip, kVoices> clips_{};
    std::array<SDL_AudioStream*, kVoices> streams_{};
    int lastCountdown_ = -1;
    bool wasRacing_ = false;
    bool wasBlocked_ = false;
    bool wasOnFire_ = false;
    bool open_ = false;
    bool menu_ = false;
    bool paused_ = false;
    std::array<int, 7> menuCursor_{};   ///< phase, rows and choices
    int menuTruguts_ = -1;
    std::filesystem::path wavs_;
    RacerPhase lastPhase_ = RacerPhase::Menu;
    std::string lastNotice_;
    int lastRacer_ = -1;
    int wattoVisits_ = 0;
    int refusals_ = 0;
};

}  // namespace sdl3cpp::services::impl
