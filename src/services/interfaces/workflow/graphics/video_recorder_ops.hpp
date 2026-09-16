#pragma once

#include "services/interfaces/workflow/graphics/video_recorder.hpp"

#include <SDL3/SDL_video.h>

namespace sdl3cpp::services::impl {

/// The context's recorder, made from @p step's settings on first use.
VideoRecorder& GetVideoRecorder(WorkflowContext& context,
                                const WorkflowStepDefinition& step,
                                const std::shared_ptr<ILogger>& logger);

/// Opens the file at the frame's size; on error, marks it done.
bool StartVideoRecorder(VideoRecorder& rec, WorkflowContext& context);

/// Finishes the file and releases the GPU objects. Idempotent.
void StopVideoRecorder(VideoRecorder& rec, WorkflowContext& context);

/// The texture a recorded frame draws into, sized to the window.
SDL_GPUTexture* EnsureVideoTarget(VideoRecorder& rec, SDL_Window* window,
                                  std::uint32_t width,
                                  std::uint32_t height);

/// Blits the target, which a recorded frame drew into, to the window.
void PresentVideoTarget(VideoRecorder& rec, SDL_GPUCommandBuffer* cmd,
                        SDL_GPUTexture* swapchain);

/// Whether @p format downloads as 8-bit BGRA or RGBA, and which.
bool VideoByteOrder(SDL_GPUTextureFormat format, bool& bgra);

/// Listens to every audio device the game has published.
void WatchVideoAudio(VideoRecorder& rec, const WorkflowContext& context);

/// Keeps what the devices play from now on; once, as recording starts.
void StartVideoAudio(VideoRecorder& rec);

/// The pts this frame records at, or -1 when none is due. Ends the
/// recording once its `seconds` are up.
std::int64_t DueVideoPts(VideoRecorder& rec, WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
