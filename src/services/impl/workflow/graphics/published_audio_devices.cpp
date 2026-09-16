#include "services/interfaces/workflow/graphics/published_audio_devices.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

constexpr const char* kKey = "audio.devices";
using Devices              = std::vector<SDL_AudioDeviceID>;

}  // namespace

void PublishAudioDevice(WorkflowContext& context,
                        SDL_AudioDeviceID device) {
    if (device == 0) return;
    Devices devices = context.Get<Devices>(kKey, Devices{});
    if (std::find(devices.begin(), devices.end(), device) ==
        devices.end()) {
        devices.push_back(device);
        context.Set<Devices>(kKey, devices);
    }
}

Devices PublishedAudioDevices(const WorkflowContext& context) {
    return context.Get<Devices>(kKey, Devices{});
}

}  // namespace sdl3cpp::services::impl
