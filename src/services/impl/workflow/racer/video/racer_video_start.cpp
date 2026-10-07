#include "services/interfaces/workflow/racer/video/racer_video_step.hpp"

#include "services/interfaces/workflow/racer/audio/racer_audio_clip.hpp"
#include "services/interfaces/workflow/racer/racer_step_params.hpp"

#include <SDL3/SDL_init.h>

namespace sdl3cpp::services::impl {

bool WorkflowRacerVideoStep::StartNext(const WorkflowStepDefinition& step,
                                       SDL_GPUDevice* device,
                                       SDL_GPUCommandBuffer* cmd,
                                       float aspect) {
    RacerFlow& flow = state_->flow;
    while (!flow.videos.empty()) {
        playing_ = flow.videos.front();
        flow.videos.erase(flow.videos.begin());
        const auto file =
            std::filesystem::path(
                RacerStringParam(step, "racer_dir", "RACER_DIR", "")) /
            "data" / "anims" / (playing_ + ".znm");
        auto bytes = ReadRacerAnim(file);
        decoder_ = std::make_unique<RacerVideoDecoder>();
        if (bytes && decoder_->Open(std::move(*bytes))) break;
        if (logger_) logger_->Warn("racer.video.play: cannot play " + playing_);
        decoder_.reset();
    }
    if (!decoder_) return false;
    const int w = decoder_->Width(), h = decoder_->Height();
    if (panel_.width != w || panel_.height != h) {
        DestroyRacerPanel(panel_);
        if (!CreateRacerTextPanel(device, w, h, panel_)) return false;
        AddRacerPanelSmoothSampler(panel_);
    }
    // Letterboxed across the full width, as the original showed it.
    RacerScreenRect rect;
    rect.top = std::min(1.f, aspect * h / static_cast<float>(w));
    rect.bottom = -rect.top;
    UploadRacerPanelRect(panel_, cmd, rect);
    if (RacerAudioAllowed() && decoder_->SampleRate() > 0 &&
        SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        const SDL_AudioSpec spec{SDL_AUDIO_S16LE, 2, decoder_->SampleRate()};
        audio_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                           &spec, nullptr, nullptr);
        if (audio_) SDL_ResumeAudioStreamDevice(audio_);
        if (!audio_) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
    if (logger_) {
        logger_->Info("racer.video.play: " + playing_ + ", " +
                      std::to_string(w) + "x" + std::to_string(h) + " at " +
                      std::to_string(decoder_->FramesPerSecond()) + " fps" +
                      (audio_ ? ", with sound" : ", silent"));
    }
    return true;
}

void WorkflowRacerVideoStep::Stop() {
    if (audio_) {
        SDL_DestroyAudioStream(audio_);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        audio_ = nullptr;
    }
    decoder_.reset();
    clock_ = 0.0;
    shown_ = 0;
}

void WorkflowRacerVideoStep::FeedAudio() {
    const std::vector<std::int16_t> samples = decoder_->TakeAudio();
    if (!audio_ || samples.empty()) return;
    SDL_PutAudioStreamData(audio_, samples.data(),
                           static_cast<int>(samples.size() * 2));
}

}  // namespace sdl3cpp::services::impl
