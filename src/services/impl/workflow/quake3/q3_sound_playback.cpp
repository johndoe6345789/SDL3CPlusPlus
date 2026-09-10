#include "services/interfaces/workflow/quake3/q3_sound_playback.hpp"

#include <algorithm>
#include <string>

namespace sdl3cpp::services::impl {

void ReapFinishedSoundStreams(std::vector<SDL_AudioStream*>& playing) {
    auto done = std::remove_if(
        playing.begin(), playing.end(), [](SDL_AudioStream* stream) {
            if (SDL_GetAudioStreamAvailable(stream) > 0) return false;
            SDL_UnbindAudioStream(stream);
            SDL_DestroyAudioStream(stream);
            return true;
        });
    playing.erase(done, playing.end());
}

SDL_AudioStream* PlaySoundOnDevice(const q3::Sound& sound,
                                   SDL_AudioDeviceID device,
                                   const SDL_AudioSpec& deviceSpec,
                                   const std::shared_ptr<ILogger>& logger) {
    SDL_AudioStream* stream =
        SDL_CreateAudioStream(&sound.spec, &deviceSpec);
    if (!stream) {
        if (logger) {
            logger->Warn(std::string("q3.sound.play: stream: ") +
                        SDL_GetError());
        }
        return nullptr;
    }

    SDL_PutAudioStreamData(stream, sound.pcm.data(),
                           static_cast<int>(sound.pcm.size()));
    SDL_FlushAudioStream(stream);

    if (!SDL_BindAudioStream(device, stream)) {
        if (logger) {
            logger->Warn(std::string("q3.sound.play: bind: ") +
                        SDL_GetError());
        }
        SDL_DestroyAudioStream(stream);
        return nullptr;
    }

    SDL_ResumeAudioDevice(device);
    return stream;
}

}  // namespace sdl3cpp::services::impl
