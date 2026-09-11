#include "services/interfaces/workflow/gta5/gta5_radio_step.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

void WorkflowGta5RadioStep::Tune() {
    const Gta5Station& station = stations_[station_];
    if (station.tracks.empty()) {
        loading_ = std::async(std::launch::async, [v = station.variant] {
            return SynthGta5Music(v);
        });
        return;
    }
    // Read off the frame loop: a track is tens of MB. One that will not
    // read plays the stand-in rather than trying again every frame.
    std::uniform_int_distribution<std::size_t> pick(
        0, station.tracks.size() - 1);
    loading_ = std::async(std::launch::async,
                          [path = station.tracks[pick(rng_)]] {
                              Gta5Clip clip = LoadGta5Track(path);
                              if (clip.pcm.empty()) clip = SynthGta5Music(0);
                              return clip;
                          });
}

void WorkflowGta5RadioStep::Play(Gta5Clip track) {
    Stop();
    stream_ = SDL_CreateAudioStream(&track.spec, &spec_);
    if (!stream_) return;
    if (!SDL_BindAudioStream(device_, stream_)) {
        Stop();
        return;
    }
    SDL_SetAudioStreamGain(stream_, volume_);
    // Tuning in finds a station mid-song, somewhere in its first 70%.
    const std::size_t frame = SDL_AUDIO_FRAMESIZE(track.spec);
    std::size_t from = 0;
    if (midway_ && frame > 0) {
        const float at = std::uniform_real_distribution<float>(0.f, .7f)(rng_);
        from = static_cast<std::size_t>(at * track.pcm.size()) / frame * frame;
    }
    midway_ = false;
    SDL_PutAudioStreamData(stream_, track.pcm.data() + from,
                           static_cast<int>(track.pcm.size() - from));
    SDL_FlushAudioStream(stream_);
}

void WorkflowGta5RadioStep::Stop() {
    if (!stream_) return;
    SDL_DestroyAudioStream(stream_);  // unbinds it too
    stream_ = nullptr;
}

}  // namespace sdl3cpp::services::impl
