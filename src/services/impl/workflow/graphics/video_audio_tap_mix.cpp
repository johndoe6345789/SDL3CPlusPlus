#include "services/interfaces/workflow/graphics/video_audio_tap.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

// Ten seconds of 48 kHz stereo: past that the encoder has stalled,
// and more is dropped rather than held.
constexpr std::size_t kMaxKept = 48000 * 2 * 10;

}  // namespace

void SDLCALL VideoAudioTap::OnMix(void* user, const SDL_AudioSpec* spec,
                                  float* buffer, int bytes) {
    const auto* source = static_cast<Source*>(user);
    const int count    = bytes / int(sizeof(float));
    source->tap->Mix(source->device, *spec, buffer, count);
}

void VideoAudioTap::Mix(SDL_AudioDeviceID device, const SDL_AudioSpec& spec,
                        const float* samples, int count) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_) return;
    if (spec.freq != rate_ || spec.channels != channels_) {
        FinishPass();
        kept_.clear();  // Take reports one format for all it returns
        rate_     = spec.freq;
        channels_ = spec.channels;
    }
    const bool seen =
        std::find(inPass_.begin(), inPass_.end(), device) != inPass_.end();
    if (seen || (!pass_.empty() && pass_.size() != std::size_t(count))) {
        FinishPass();
    }
    if (pass_.empty()) pass_.assign(count, 0.f);
    for (int i = 0; i < count; ++i) {
        pass_[i] += samples[i];
    }
    inPass_.push_back(device);
}

void VideoAudioTap::FinishPass() {
    if (!pass_.empty() && kept_.size() < kMaxKept) {
        kept_.insert(kept_.end(), pass_.begin(), pass_.end());
    }
    pass_.clear();
    inPass_.clear();
}

}  // namespace sdl3cpp::services::impl
