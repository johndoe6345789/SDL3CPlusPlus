#pragma once

#include <SDL3/SDL_audio.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::q3 {

/// One decoded sound, held in memory. Quake sounds are short, so they
/// are kept whole rather than streamed; nothing touches disk after the
/// pk3 is read.
struct Sound {
    SDL_AudioSpec spec{};
    std::vector<uint8_t> pcm;
};

using SoundBank = std::unordered_map<std::string, Sound>;

/// Quake references sounds by their path inside the pk3, e.g.
/// "sound/weapons/machinegun/machgf1b.wav". ioq3 registers them by that
/// same string (trap_S_RegisterSound), so the bank is keyed on it.
using SoundBankPtr = std::shared_ptr<SoundBank>;

/**
 * @brief Decode one WAV held in memory into a Sound.
 *
 * @return false when the bytes are not a WAV SDL can read, leaving the
 *         sound untouched so a bad entry cannot poison the bank.
 */
bool DecodeWav(const uint8_t* bytes, size_t size, Sound& out);

/// Number of playable entries, ignoring any that failed to decode.
size_t CountPlayable(const SoundBank& bank);

}  // namespace sdl3cpp::q3
