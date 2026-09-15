#pragma once

#include "services/interfaces/i_logger.hpp"

#include <SDL3/SDL_audio.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Decodes `loopPath` from `pk3`, creates an audio stream for it,
 * queues an optional `introPath` ahead of the loop (ioq3 plays the intro
 * once before looping), and queues the loop itself.
 *
 * On success, sets `outStream` to the new stream (caller owns it) and
 * `outLoopPcm` to the loop's decoded PCM (kept around so q3.music.play can
 * re-queue it as the stream drains). Returns false — leaving both outputs
 * untouched — if the loop track can't be read/decoded or the stream can't
 * be created; logs a Warn via `logger` (which may be null) when the loop
 * is missing.
 */
bool LoadQ3MusicTracks(const std::string& pk3, const std::string& introPath,
                       const std::string& loopPath,
                       const SDL_AudioSpec& deviceSpec,
                       const std::shared_ptr<ILogger>& logger,
                       SDL_AudioStream*& outStream,
                       std::vector<uint8_t>& outLoopPcm);

}  // namespace sdl3cpp::services::impl
