#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_audio.h>

#include <vector>

namespace sdl3cpp::services::impl {

/// Tells the rest of the frame that @p device plays the game's sound,
/// so a recording can capture it. Zero is ignored.
void PublishAudioDevice(WorkflowContext& context, SDL_AudioDeviceID device);

/// Every device published so far.
std::vector<SDL_AudioDeviceID> PublishedAudioDevices(
    const WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
