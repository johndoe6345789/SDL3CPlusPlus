#include "services/interfaces/workflow/graphics/video_audio_tap.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

// Clearing a callback waits for SDL's device lock, so for any mix in
// progress: after this, none can reach a freed tap. Not under mutex_,
// which that mix may be waiting on.
VideoAudioTap::~VideoAudioTap() {
    for (const auto& source : sources_) {
        SDL_SetAudioPostmixCallback(source->device, nullptr, nullptr);
    }
}

void VideoAudioTap::Watch(SDL_AudioDeviceID device) {
    for (const auto& source : sources_) {
        if (source->device == device) return;
    }
    auto source = std::make_unique<Source>(Source{this, device});
    if (SDL_SetAudioPostmixCallback(device, &OnMix, source.get())) {
        sources_.push_back(std::move(source));
    }
}

void VideoAudioTap::SetActive(bool active) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active) FinishPass();
    active_ = active;
}

bool VideoAudioTap::Take(std::vector<float>& samples, int& rate,
                         int& channels) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (kept_.empty()) return false;
    samples.swap(kept_);
    kept_.clear();
    rate     = rate_;
    channels = channels_;
    return true;
}

}  // namespace sdl3cpp::services::impl
