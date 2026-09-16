#include "services/interfaces/workflow/graphics/video_encode_worker.hpp"

#include <chrono>
#include <optional>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

// Audio arrives whether or not a frame does; this often, it is taken.
constexpr std::chrono::milliseconds kAudioPoll{20};

}  // namespace

void VideoEncodeWorker::Run() {
    for (;;) {
        std::optional<VideoFrame> frame;
        bool stop = false;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            wake_.wait_for(lock, kAudioPoll,
                           [this] { return stopping_ || !queue_.empty(); });
            // What was queued before stopping is the recording's end,
            // so the queue empties before the thread does.
            if (!queue_.empty()) {
                frame = std::move(queue_.front());
                queue_.pop_front();
            } else {
                stop = stopping_;
            }
        }
        PumpAudio();
        if (frame && encoder_.Write(*frame)) ++written_;
        if (stop) return;
    }
}

}  // namespace sdl3cpp::services::impl
