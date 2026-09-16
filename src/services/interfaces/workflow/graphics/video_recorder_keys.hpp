#pragma once

namespace sdl3cpp::services::impl {

/// The recorder's own context entry (a shared_ptr<VideoRecorder>).
inline constexpr const char* kVideoRecorderKey = "video_recorder";

/// Set by video.record.end. gpu.command_buffer_submit then submits
/// with a fence and leaves it under kGpuSubmitFenceKey, for the
/// recorder to learn when its download has arrived.
inline constexpr const char* kGpuSubmitFenceWantedKey =
    "gpu_submit_fence_wanted";
inline constexpr const char* kGpuSubmitFenceKey = "gpu_submit_fence";

}  // namespace sdl3cpp::services::impl
