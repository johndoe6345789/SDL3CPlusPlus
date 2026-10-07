#include "services/interfaces/workflow/racer/audio/racer_audio_clip.hpp"

#include <cstdlib>
#include <string>

namespace sdl3cpp::services::impl {
namespace {

bool EnvSet(const char* name) {
    const char* value = std::getenv(name);
    return value && value[0] != 0 && std::string(value) != "0";
}

}  // namespace

/// Headless dev runs stay silent unless SDL's dummy driver is chosen,
/// which plays to nowhere and so checks the audio path.
bool RacerAudioAllowed() {
    const char* driver = std::getenv("SDL_AUDIO_DRIVER");
    const bool dummy = driver && std::string(driver) == "dummy";
    return !(EnvSet("SDL3CPP_HEADLESS") && !dummy) && !EnvSet("RACER_MUTE");
}

}  // namespace sdl3cpp::services::impl
