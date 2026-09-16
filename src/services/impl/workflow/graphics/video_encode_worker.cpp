#include "services/interfaces/workflow/graphics/video_encode_worker.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

VideoEncodeWorker::~VideoEncodeWorker() {
    Finish();
}

std::string VideoEncodeWorker::Start(const VideoEncoderSettings& settings) {
    std::string error = encoder_.Open(settings);
    if (!error.empty()) return error;
    codec_  = encoder_.CodecName();
    thread_ = std::thread([this] { Run(); });
    return "";
}

bool VideoEncodeWorker::Push(VideoFrame frame) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!thread_.joinable() || queue_.size() >= kMaxQueued) {
            return false;
        }
        queue_.push_back(std::move(frame));
    }
    wake_.notify_one();
    return true;
}

void VideoEncodeWorker::Finish() {
    if (!thread_.joinable()) return;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    wake_.notify_one();
    thread_.join();
    encoder_.Close();
    audio_.reset();
}

}  // namespace sdl3cpp::services::impl
