#include "services/interfaces/workflow/quake3/q3_sound_bank.hpp"

#include <SDL3/SDL_iostream.h>

#include <algorithm>

namespace sdl3cpp::q3 {

bool DecodeWav(const uint8_t* bytes, size_t size, Sound& out) {
    if (!bytes || size == 0) {
        return false;
    }
    SDL_IOStream* io = SDL_IOFromConstMem(bytes, size);
    if (!io) {
        return false;
    }

    SDL_AudioSpec spec{};
    uint8_t* pcm = nullptr;
    uint32_t length = 0;
    // closeio: SDL takes the stream even on failure.
    const bool ok = SDL_LoadWAV_IO(io, true, &spec, &pcm, &length);
    if (!ok || !pcm) {
        return false;
    }

    out.spec = spec;
    out.pcm.assign(pcm, pcm + length);
    SDL_free(pcm);
    return true;
}

size_t CountPlayable(const SoundBank& bank) {
    return static_cast<size_t>(std::count_if(
        bank.begin(), bank.end(),
        [](const auto& entry) { return !entry.second.pcm.empty(); }));
}

}  // namespace sdl3cpp::q3
