#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/graphics/video_audio_tap.hpp"
#include "services/interfaces/workflow/graphics/video_encode_worker.hpp"
#include "services/interfaces/workflow/graphics/video_readback.hpp"
#include "services/interfaces/workflow/graphics/video_recorder_keys.hpp"
#include "services/interfaces/workflow/graphics/video_recorder_settings.hpp"

#include <SDL3/SDL_gpu.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace sdl3cpp::services::impl {

/// What video.record.* share, kept in the context under
/// kVideoRecorderKey. Functions on it are in video_recorder_ops.hpp.
struct VideoRecorder {
    VideoRecorderSettings settings;
    std::shared_ptr<ILogger> logger;
    SDL_GPUDevice* device = nullptr;
    bool bgra             = true;
    // Drawn into in place of the swapchain on a frame being recorded.
    SDL_GPUTexture* target     = nullptr;
    std::uint32_t targetWidth  = 0;
    std::uint32_t targetHeight = 0;
    SDL_GPUTexture* swapchain  = nullptr;  // while target stands in
    std::int64_t capturePts    = 0;
    std::array<VideoReadback, 3> readbacks;
    VideoEncodeWorker worker;
    std::shared_ptr<VideoAudioTap> audio;  // null: no audio track
    bool audioLive        = false;
    std::uint64_t startNs = 0;
    std::int64_t nextPts  = 0;
    bool started          = false;
    bool done             = false;
    std::size_t dropped   = 0;
};

}  // namespace sdl3cpp::services::impl
