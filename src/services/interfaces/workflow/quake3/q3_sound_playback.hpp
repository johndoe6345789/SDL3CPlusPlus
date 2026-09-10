#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/quake3/q3_sound_bank.hpp"

#include <SDL3/SDL_audio.h>

#include <memory>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Removes and destroys every stream in `playing` that has fully
 *        drained.
 *
 * A sound that has been fully consumed is done; without this every shot
 * leaks a stream for the lifetime of the map.
 */
void ReapFinishedSoundStreams(std::vector<SDL_AudioStream*>& playing);

/**
 * @brief Creates a stream for `sound`, feeds it that sound's PCM data,
 *        and binds it to `device`.
 *
 * One stream per sound played: SDL mixes everything bound to the
 * device, so overlapping shots need no mixing of our own.
 *
 * @return nullptr if stream creation or binding fails, after logging a
 *         warning through `logger`.
 */
SDL_AudioStream* PlaySoundOnDevice(const q3::Sound& sound,
                                   SDL_AudioDeviceID device,
                                   const SDL_AudioSpec& deviceSpec,
                                   const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
