#include "services/interfaces/workflow/quake3/q3_music_helpers.hpp"
#include "services/interfaces/workflow/quake3/q3_pk3_reader.hpp"
#include "services/interfaces/workflow/quake3/q3_sound_bank.hpp"

namespace sdl3cpp::services::impl {

bool LoadQ3MusicTracks(const std::string& pk3, const std::string& introPath,
                       const std::string& loopPath,
                       const SDL_AudioSpec& deviceSpec,
                       const std::shared_ptr<ILogger>& logger,
                       SDL_AudioStream*& outStream,
                       std::vector<uint8_t>& outLoopPcm) {
    q3::Sound loop;
    const auto loopBytes = q3::ReadPk3Entry(pk3, loopPath);
    if (!q3::DecodeWav(loopBytes.data(), loopBytes.size(), loop)) {
        if (logger) logger->Warn("q3.music.play: no loop track");
        return false;
    }

    SDL_AudioStream* stream = SDL_CreateAudioStream(&loop.spec, &deviceSpec);
    if (!stream) return false;

    // The intro plays once ahead of the loop, as ioq3 does.
    q3::Sound intro;
    const auto introBytes = q3::ReadPk3Entry(pk3, introPath);
    if (q3::DecodeWav(introBytes.data(), introBytes.size(), intro)) {
        SDL_PutAudioStreamData(stream, intro.pcm.data(),
                               static_cast<int>(intro.pcm.size()));
    }

    outLoopPcm = std::move(loop.pcm);
    SDL_PutAudioStreamData(stream, outLoopPcm.data(),
                           static_cast<int>(outLoopPcm.size()));

    outStream = stream;
    return true;
}

}  // namespace sdl3cpp::services::impl
