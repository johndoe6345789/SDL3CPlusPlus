#include "services/interfaces/workflow/graphics/video_encode_worker.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

void VideoEncodeWorker::SetAudio(std::shared_ptr<VideoAudioTap> tap) {
    audio_ = std::move(tap);
}

void VideoEncodeWorker::PumpAudio() {
    int rate = 0, channels = 0;
    if (!audio_ || !audio_->Take(audioBuffer_, rate, channels)) return;
    if (channels <= 0) return;
    const int frames = int(audioBuffer_.size()) / channels;
    encoder_.WriteAudio(audioBuffer_.data(), frames, rate, channels);
}

std::string VideoEncodeWorker::CodecName() const {
    return codec_;
}

std::size_t VideoEncodeWorker::Written() const {
    return written_;
}

}  // namespace sdl3cpp::services::impl
