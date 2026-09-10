#pragma once

#include "services/interfaces/i_audio_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/media_types.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Validates `selection`'s path and, for a "play" action, plays it
 *        through `audioService`; other actions are reported as unknown.
 *
 * Mirrors the status/logging behavior media.item.select previously had
 * inline: a missing path, a nonexistent file, a playback exception, and
 * an unrecognized action are all reported as a human-readable status
 * string rather than thrown, and each case is also logged when a
 * logger is present.
 */
std::string DispatchMediaSelection(IAudioService& audioService,
                                    ILogger* logger,
                                    const std::string& action,
                                    const MediaSelection& selection);

}  // namespace sdl3cpp::services::impl
