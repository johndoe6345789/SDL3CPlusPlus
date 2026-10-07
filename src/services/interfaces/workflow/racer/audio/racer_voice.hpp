#pragma once

#include "services/interfaces/workflow/racer/audio/racer_audio_clip.hpp"
#include "services/interfaces/workflow/racer/audio/racer_voice_lines.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <filesystem>
#include <map>
#include <string>

namespace sdl3cpp::services::impl {

/// The player's racer talking during a race: reacts to contact, walls,
/// jumps, fire, damage, repairs, overtaking and the finish, at most one
/// line every few seconds.
class RacerVoice {
public:
    RacerVoice(std::filesystem::path racerDir, std::string voice);
    ~RacerVoice();
    RacerVoice(const RacerVoice&) = delete;
    RacerVoice& operator=(const RacerVoice&) = delete;

    void Update(const RacerWorldState& state, float dt);
    void SetPaused(bool paused);

private:
    void Say(RacerVoiceEvent event, bool urgent);

    std::filesystem::path dir_;
    std::string voice_;
    std::map<std::string, RacerAudioClip> clips_;
    SDL_AudioStream* stream_ = nullptr;
    float quiet_ = 1.f;       ///< seconds until the next line may play
    float tauntQuiet_ = 0.f;
    int said_ = 0;            ///< lines said, for variety
    int lastPosition_ = 0;
    float lastDamage_ = 0.f;
    bool wasAir_ = false, wasBlocked_ = false, wasFire_ = false;
    bool wasRacing_ = false, wasFinished_ = false, hurt_ = false;
};

}  // namespace sdl3cpp::services::impl
