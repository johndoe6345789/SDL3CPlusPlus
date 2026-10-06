#include "services/interfaces/workflow/racer/audio/racer_audio_clip.hpp"

#include "services/interfaces/workflow/racer/data/racer_asset_io.hpp"
#include "services/interfaces/workflow/racer/data/racer_wav.hpp"

namespace sdl3cpp::services::impl {

RacerAudioClip LoadRacerAudioClip(const std::filesystem::path& path) {
    RacerAudioClip clip;
    const auto file = ReadRacerFile(path);
    RacerPcm pcm;
    if (!file || !ReadRacerWav(*file, pcm) || pcm.samples.empty()) {
        return clip;
    }
    clip.spec.format = SDL_AUDIO_S16LE;
    clip.spec.channels = pcm.channels;
    clip.spec.freq = static_cast<int>(pcm.sampleRate);
    const auto* raw =
        reinterpret_cast<const std::uint8_t*>(pcm.samples.data());
    clip.bytes.assign(raw, raw + pcm.samples.size() * sizeof(std::int16_t));
    return clip;
}

SDL_AudioStream* OpenRacerAudioStream(const RacerAudioClip& clip) {
    if (!clip.Loaded()) return nullptr;
    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &clip.spec, nullptr, nullptr);
    if (stream) SDL_ResumeAudioStreamDevice(stream);
    return stream;
}

void FeedRacerAudioLoop(SDL_AudioStream* stream, const RacerAudioClip& clip) {
    if (!stream || !clip.Loaded()) return;
    const int bytesPerSecond = clip.spec.freq * clip.spec.channels * 2;
    if (SDL_GetAudioStreamQueued(stream) < bytesPerSecond / 2) {
        SDL_PutAudioStreamData(stream, clip.bytes.data(),
                               static_cast<int>(clip.bytes.size()));
    }
}

void PlayRacerAudioOnce(SDL_AudioStream* stream, const RacerAudioClip& clip) {
    if (!stream || !clip.Loaded()) return;
    SDL_ClearAudioStream(stream);
    SDL_PutAudioStreamData(stream, clip.bytes.data(),
                           static_cast<int>(clip.bytes.size()));
}

}  // namespace sdl3cpp::services::impl
