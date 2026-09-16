#include "services/interfaces/workflow/graphics/video_recorder_ops.hpp"
#include "services/interfaces/workflow/graphics/published_audio_devices.hpp"

namespace sdl3cpp::services::impl {

// Every frame: a device can open after the recording has started, as
// the radio's does on first getting into a car.
void WatchVideoAudio(VideoRecorder& rec, const WorkflowContext& context) {
    if (!rec.audio) return;
    for (SDL_AudioDeviceID device : PublishedAudioDevices(context)) {
        rec.audio->Watch(device);
    }
}

void StartVideoAudio(VideoRecorder& rec) {
    if (!rec.audio || rec.audioLive) return;
    rec.audio->SetActive(true);
    rec.audioLive = true;
}

}  // namespace sdl3cpp::services::impl
