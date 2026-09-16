#pragma once

#include "services/interfaces/workflow/graphics/video_audio_tap.hpp"
#include "services/interfaces/workflow/graphics/video_encoder.hpp"

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Runs a VideoEncoder on its own thread, so the frame loop
 * does not pay the milliseconds converting and encoding take.
 *
 * More than `kMaxQueued` frames behind, new frames are dropped.
 * Between frames it takes what the audio tap has kept.
 */
class VideoEncodeWorker {
public:
    static constexpr std::size_t kMaxQueued = 8;

    ~VideoEncodeWorker();

    /// Where the audio comes from; before Start, or never.
    void SetAudio(std::shared_ptr<VideoAudioTap> tap);
    /// Opens the file on the calling thread, then starts the thread.
    std::string Start(const VideoEncoderSettings& settings);
    /// @return false when the frame was dropped.
    bool Push(VideoFrame frame);
    /// Encodes what is queued, closes the file, joins the thread.
    void Finish();

    std::string CodecName() const;
    /// Frames in the file. Only meaningful after Finish.
    std::size_t Written() const;

private:
    void Run();
    void PumpAudio();

    VideoEncoder encoder_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable wake_;
    std::deque<VideoFrame> queue_;
    bool stopping_       = false;
    std::size_t written_ = 0;  // the worker's, until Finish joins it
    std::string codec_;
    std::shared_ptr<VideoAudioTap> audio_;
    std::vector<float> audioBuffer_;
};

}  // namespace sdl3cpp::services::impl
